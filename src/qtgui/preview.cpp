/*
    This file is part of Mitsuba, a physically based rendering system.

    Copyright (c) 2007-2011 by Wenzel Jakob and others.

    Mitsuba is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Mitsuba is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <GL/glew.h>

#include "glwidget.h"
#include <mitsuba/core/timer.h>
#include "preview.h"
#include <mitsuba/hw/gltexture.h>
#include <mitsuba/core/bitmap.h>
#include <mitsuba/core/mstream.h>
#include <mitsuba/core/fstream.h>
#include <mitsuba/core/fresolver.h>

//GLEWContext glewContext;

//GLEWContext *glewGetContext() {
//    return &glewContext;
//}

unsigned int PreviewThread::intColFormRGBF[2] = {GL_RGB16F_ARB, GL_RGB16F_ARB};
unsigned int PreviewThread::intColFormRGBAF[2] = {GL_RGBA16F_ARB, GL_RGBA16F_ARB};
unsigned int PreviewThread::intColFormRGBAF32[2] = {GL_RGBA32F_ARB, GL_RGBA32F_ARB};
unsigned int PreviewThread::filter[2] = {GL_NEAREST, GL_NEAREST};
unsigned int PreviewThread::filterL[2] = {GL_LINEAR, GL_LINEAR};
unsigned int PreviewThread::wrap[2] = {GL_CLAMP, GL_CLAMP};
unsigned int PreviewThread::wrapE[2] = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};

PreviewThread::PreviewThread(Device *parentDevice, Renderer *parentRenderer)
	: Thread("prev"), m_parentDevice(parentDevice), m_parentRenderer(parentRenderer), 
		m_directShaderManager(NULL), m_context(NULL), m_quit(false) {
	MTS_AUTORELEASE_BEGIN()
	m_session = Session::create();
	m_device = Device::create(m_session);
	m_renderer = Renderer::create(m_session);
	m_mutex = new Mutex();
	m_queueCV = new ConditionVariable(m_mutex);
	m_bufferCount = 3;
	m_backgroundScaleFactor = 1.0f;
	m_queueEntryIndex = 0;
	m_session->init();
	m_timer = new Timer();
	m_accumBuffer = NULL;
	m_sleep = false;
	m_started = new WaitFlag();

    /* Create accumulation program to combine several rendering results
     * (available as textures/FBO's) by adding them up. This is mainly
     * needed for VPL rendering. */
	m_accumProgram = m_renderer->createGPUProgram("Accumulation program");
	m_accumProgram->setSource(GPUProgram::EVertexProgram,
		"void main() {\n"
		"	gl_Position = ftransform();\n"
		"   gl_TexCoord[0]  = gl_MultiTexCoord0;\n"
		"}\n"
	);

	m_accumProgram->setSource(GPUProgram::EFragmentProgram,
		"uniform sampler2D source1, source2;\n"
		"void main() {\n"
		"	gl_FragColor = texture2D(source1, gl_TexCoord[0].xy) + \n"
		"                  texture2D(source2, gl_TexCoord[0].xy);\n"
		"}\n"
	);
				
	m_framebuffer = m_renderer->createGPUTexture("Framebuffer");
	for (int i=0; i<m_bufferCount; ++i) 
		m_recycleQueue.push_back(PreviewQueueEntry(m_queueEntryIndex++));
	
	m_random = new Random();

    /* create FBOs for realtime SSS */
    fboLightView  = new FrameBufferObject(2);
    fboView  = new FrameBufferObject(1);
    fboViewExpand  = new FrameBufferObject(1);
    fboCumulSplat = new FrameBufferObject(1);
    fboTmp = new FrameBufferObject(1);
    splatOrigins = NULL;
    splatColors = NULL;

    fboCumulSplatWidth = -1;
    fboCumulSplatHeight = -1;

	MTS_AUTORELEASE_END()
}

PreviewThread::~PreviewThread() {
	MTS_AUTORELEASE_BEGIN()
	m_session->shutdown();
	MTS_AUTORELEASE_END()
}

void PreviewThread::quit() {
	if (!isRunning())
		return;

	std::vector<PreviewQueueEntry> temp;
	temp.reserve(m_bufferCount);
	
	/* Steal all buffers */
	m_mutex->lock();
	while (true) {
		while (m_readyQueue.size() > 0) {
			temp.push_back(m_readyQueue.back());
			m_readyQueue.pop_back();
		}

		while (m_recycleQueue.size() > 0) {
			temp.push_back(m_recycleQueue.back());
			m_recycleQueue.pop_back();
		}

		if ((int) temp.size() == m_bufferCount)
			break;

		m_queueCV->wait();
	}

	/* Put back all buffers and disassociate */
	for (size_t i=0; i<temp.size(); ++i) {
		if (temp[i].buffer)
			temp[i].buffer->disassociate();
		m_recycleQueue.push_back(temp[i]);
	}
	m_quit = true;

	m_queueCV->signal();
	m_mutex->unlock();

	/* Wait for the thread to terminate */
	if (isRunning())
		join();
}

void PreviewThread::setSceneContext(SceneContext *context, bool swapContext, bool motion) {
	if (!isRunning())
		return;

	std::vector<PreviewQueueEntry> temp;
	temp.reserve(m_bufferCount);

	m_sleep = true;
	m_mutex->lock();

	/* Steal all buffers from the rendering
	   thread to make sure we get its attention :) */
	while (true) {
		while (m_readyQueue.size() > 0) {
			temp.push_back(m_readyQueue.front());
				m_readyQueue.pop_front();
		}

		while (m_recycleQueue.size() > 0) {
			temp.push_back(m_recycleQueue.back());
			m_recycleQueue.pop_back();
		}

		if ((int) temp.size() == m_bufferCount)
			break;

		m_queueCV->wait();
	}

	if (swapContext && m_context) {
		m_context->vpls = m_vpls;
		m_context->previewBuffer = temp[0];
		if (m_context->previewBuffer.buffer)
			m_context->previewBuffer.buffer->disassociate();
		m_recycleQueue.push_back(PreviewQueueEntry(m_queueEntryIndex++));

		/* Put back all buffers */
		for (size_t i=1; i<temp.size(); ++i)
			m_recycleQueue.push_back(temp[i]);
	} else {
		for (size_t i=0; i<temp.size(); ++i)
			m_recycleQueue.push_back(temp[i]);
	}

	if (swapContext && context && context->previewBuffer.vplSampleOffset > 0) {
		/* Resume from a stored state */
		m_vplSampleOffset = context->previewBuffer.vplSampleOffset;
		m_vpls = context->vpls;
		m_accumBuffer = context->previewBuffer.buffer;

		/* Take ownership of the buffer */
		m_recycleQueue.push_back(context->previewBuffer);
		context->previewBuffer.buffer = NULL;
		context->previewBuffer.sync = NULL;
		context->previewBuffer.vplSampleOffset = 0;

		if (m_recycleQueue.size() > (size_t) m_bufferCount) {
			PreviewQueueEntry entry = m_recycleQueue.front();
			m_recycleQueue.pop_front();
			if (entry.buffer) {
				entry.buffer->disassociate();
				entry.buffer->decRef();
			}
			if (entry.sync) 
				entry.sync->decRef();
		}
	} else {
		/* Reset the VPL rendering progress */
		m_vplSampleOffset = 0;
		m_vpls.clear();
		m_accumBuffer = NULL;
	}

	if (m_context != context)
		m_minVPLs = 0;

	m_vplsPerSecond = 0;
	m_raysPerSecond = 0;
	m_vplCount = 0;
	m_timer->reset();	

	m_context = context;

	if (m_context) {
		Camera *camera = m_context->scene->getCamera();
		m_camPos = camera->getPosition();
		m_camViewTransform = camera->getViewTransform();
	}

	if (motion && !m_motion) {
		emit statusMessage("");
		m_minVPLs = 0;
	}

	m_motion = motion;
	m_queueCV->signal();
	m_mutex->unlock();
	m_sleep = false;
}

void PreviewThread::resume() {
	m_queueCV->signal();
}

PreviewQueueEntry PreviewThread::acquireBuffer(int ms) {
	PreviewQueueEntry entry;

	m_mutex->lock();
	while (m_readyQueue.size() == 0) {
		if (m_quit)
			return entry;
		if (!m_queueCV->wait(ms)) {
			m_mutex->unlock();
			return entry;
		}
	}
	entry = m_readyQueue.front();
	m_readyQueue.pop_front();
	m_mutex->unlock();

	if (m_context->previewMethod == ERayTrace || 
		m_context->previewMethod == ERayTraceCoherent) 
		entry.buffer->refresh();
	else if (m_useSync) 
		entry.sync->enqueueWait(); 

	return entry;
}

void PreviewThread::releaseBuffer(PreviewQueueEntry &entry) {
	m_mutex->lock();

	if (m_motion)
		m_readyQueue.push_front(entry);
	else
		m_recycleQueue.push_back(entry);

	if (m_useSync)
		entry.sync->cleanup();

	m_queueCV->signal();
	m_mutex->unlock();
}

void PreviewThread::run() {
	MTS_AUTORELEASE_BEGIN()

	bool initializedGraphics = false;

	try {
		m_device->init(m_parentDevice);
		m_device->setVisible(false);

		/* We have alrady seen this once */
		m_renderer->setLogLevel(ETrace);
		m_renderer->setWarnLogLevel(ETrace);
		m_renderer->init(m_device, m_parentRenderer);
		m_renderer->setLogLevel(EDebug);
		m_renderer->setWarnLogLevel(EWarn);
		m_started->set(true);

		m_accumProgram->init();
		m_accumProgramParam_source1 = m_accumProgram->getParameterID("source1");
		m_accumProgramParam_source2 = m_accumProgram->getParameterID("source2");
		m_useSync = m_renderer->getCapabilities()->isSupported(RendererCapabilities::ESyncObjects);

		initializedGraphics = true;

		while (true) {
			PreviewQueueEntry target;

			m_mutex->lock();
			while (!(m_quit || (m_context != NULL && m_context->mode == EPreview
					&& m_context->previewMethod != EDisabled 
					&& ((m_readyQueue.size() != 0 && !m_motion) || m_recycleQueue.size() != 0))))
				m_queueCV->wait();

			MTS_AUTORELEASE_END()
			MTS_AUTORELEASE_BEGIN()

			if (m_quit) {
				m_mutex->unlock();
				break;
			} else if (m_recycleQueue.size() != 0) {
				target = m_recycleQueue.front();
				m_recycleQueue.pop_front();
			} else if (m_readyQueue.size() != 0 && !m_motion) {
				target = m_readyQueue.front();
				m_readyQueue.pop_front();
			} else {
				Log(EError, "Internal error!");
			}

			if (m_motion && m_vplCount >= m_minVPLs && m_minVPLs != 0) {
				/* The user is currently moving around, and a good enough
				   preview has already been rendered. Don't improve it to
				   avoid flicker */
				m_recycleQueue.push_back(target);
				m_queueCV->wait();
				m_mutex->unlock();
				continue;
			}

			m_mutex->unlock();

			if (m_vplSampleOffset == 0) 
				m_accumBuffer = NULL;

			const Film *film = m_context->scene->getFilm();
			Point3i size(film->getSize().x, film->getSize().y, 1);

			if (target.buffer == NULL || target.buffer->getSize() != size) {
				target.buffer = m_renderer->createGPUTexture(formatString("Communication buffer %i", target.id));
				target.buffer->setFormat(GPUTexture::EFloat32RGB);
				target.buffer->setSize(size);
				target.buffer->setFilterType(GPUTexture::ENearest);
				target.buffer->setFrameBufferType(GPUTexture::EColorAndDepthBuffer);
				target.buffer->setMipMapped(false);
				target.buffer->init();
				target.buffer->incRef();
				target.sync = m_renderer->createGPUSync();
				target.sync->incRef();
				m_renderer->finish();
			}

			if (m_context->previewMethod == EDisabled) {
				/* Do nothing, fall asleep in the next iteration */
			} else if (m_context->previewMethod == ERayTrace || m_context->previewMethod == ERayTraceCoherent) {
				if (m_previewProc == NULL || m_previewProc->getScene() != m_context->scene) 
					m_previewProc = new PreviewProcess(m_context->scene, m_context->sceneResID, 32);

				if (m_timer->getMilliseconds() > 1000) {
					Float time = 1000 / (Float) m_timer->getMilliseconds();
					Float vplCount = m_vplsPerSecond * time, rayCount = m_raysPerSecond * time / 1000000.0f;
					if (!m_motion)
						emit statusMessage(QString(formatString("%.1f VPLs, %.1f MRays/sec", vplCount, rayCount).c_str()));
					m_vplsPerSecond = 0;
					m_raysPerSecond = 0;
					m_timer->reset();
				}

				if (m_vpls.empty()) {
					size_t oldOffset = m_vplSampleOffset;
					m_vplSampleOffset = generateVPLs(m_context->scene, m_random,
						m_vplSampleOffset, 1, m_context->pathLength, !m_motion, m_vpls);
					m_backgroundScaleFactor = m_vplSampleOffset - oldOffset;
				}

				VPL vpl = m_vpls.front();
				m_vpls.pop_front();

				rtrtRenderVPL(target, vpl);
			} else if (m_context->previewMethod == EOpenGLRealtime) {
				if (m_directShaderManager == NULL || m_directShaderManager->getScene() != m_context->scene) {
					if (m_directShaderManager) {
						m_directShaderManager->cleanup();
						m_framebuffer->cleanup();
					}
					m_directShaderManager = new DirectShaderManager(m_context->scene, m_renderer);
					m_directShaderManager->init();
					m_framebuffer->setFormat(GPUTexture::EFloat32RGB);
					m_framebuffer->setSize(size);
					m_framebuffer->setFilterType(GPUTexture::ENearest);
					m_framebuffer->setFrameBufferType(GPUTexture::EColorBuffer);
					m_framebuffer->setMipMapped(false);
					m_framebuffer->init();
                }

				m_directShaderManager->setShadowMapResolution(m_context->shadowMapResolution);
				m_directShaderManager->setClamping(m_context->clamping);
				m_directShaderManager->setSinglePass(m_context->previewMethod == EOpenGLSinglePass);
				//m_directShaderManager->setDiffuseSources(m_context->diffuseSources);
				m_directShaderManager->setDiffuseReceivers(m_context->diffuseReceivers);

				if (m_timer->getMilliseconds() > 1000) {
                    Float time = (Float) m_timer->getMilliseconds() / m_vplsPerSecond;
					Float count = 1.0 / time * 1000;
					if (!m_motion)
						emit statusMessage(QString(formatString("%.2f Frames/sec -- %.0f msec/Frame", count, time).c_str()));
					m_vplsPerSecond = 0;
					m_timer->reset();
				}

                /* read in new realtime SSS data (if changed) */
                SnowRenderSettings &srs = m_context->snowRenderSettings;
                if ((albedoMap.get() == NULL) || (srs.shahAlbedoMap.get() != albedoMap->getBitmap())) {
                    /* albedo texture */
                    if (srs.shahAlbedoMap != NULL) {
                        srs.shahAlbedoMap->incRef();
                        albedoMap = new GLTexture("Albedo Map", srs.shahAlbedoMap);
                        albedoMap->setFilterType(GPUTexture::ELinear);
                        albedoMap->setMipMapped(false);
                        albedoMap->init();
                    } else {
                        albedoMap = NULL;
                    }

#ifdef SSSDEBUG
                    std::string name = (albedoMap == NULL) ? "None" : albedoMap->toString();
                    Log(EDebug, "Set new realtime sss albedo map:");
					Log(EDebug, "\t%s", name.c_str());
#endif
                }
                if ((diffusionMap.get() == NULL) || (srs.shahDiffusionProfile.get() != diffusionMap->getBitmap()) ) {
                    /* diffusion profile / sub surface scattering texture */
                    if (srs.shahDiffusionProfile != NULL) {
                        srs.shahDiffusionProfile->incRef();
                        diffusionMap = new GLTexture("Diffusion profile map", srs.shahDiffusionProfile);
                        diffusionMap->setFilterType(GPUTexture::ELinear);
                        diffusionMap->setType(GPUTexture::ETexture2D);
                        diffusionMap->setMipMapped(false);
                        diffusionMap->init();
                    } else {
                        diffusionMap = NULL;
                    }

                    /* We also need to change the light view resolution. According to Shah et al. this is
                     * done by first calculating the number of overlapping quads n0: Rd(0) / (n0 * Rd(r_max) < err.
                     * This means n0 > Rd(0) / (err * Rd(r_max)).
                     */
                    Bitmap *dp = srs.shahDiffusionProfile;
                    size_t dpSize = dp->getSize();
                    Float Rd0Max, RdRMaxMax;
                    if (dp->getBitsPerPixel() == 128) {
                        Float *data = dp->getFloatData();
                        /* Find maximum R(0) value */
                        Rd0Max = data[0];
                        if (Rd0Max < data[1]) Rd0Max = data[1];
                        if (Rd0Max < data[2]) Rd0Max = data[2];
                        /* Find maximum R(r_max) vavue */
                        RdRMaxMax = data[dpSize - 4];
                        if (RdRMaxMax < data[dpSize - 3]) RdRMaxMax = data[dpSize - 3];
                        if (RdRMaxMax < data[dpSize - 2]) RdRMaxMax = data[dpSize - 2];
                        if (RdRMaxMax < 0.0001) RdRMaxMax = 0.0001;
                    } else {
                        unsigned char *data = dp->getData();
                        /* Find maximum R(0) value */
                        unsigned char R = data[0];
                        if (R < data[1]) R = data[1];
                        if (R < data[2]) R = data[2];
                        Rd0Max = (Float) R;
                        /* Find maximum R(r_max) value */
                        R = data[dpSize - 4];
                        if (R < data[dpSize - 3]) R = data[dpSize - 3];
                        if (R < data[dpSize - 2]) R = data[dpSize - 2];
                        RdRMaxMax = (Float) R;
                        if (RdRMaxMax < 0.0001) RdRMaxMax = 0.0001;
                    }
                    /* Calculate n0 */
                    n0 = Rd0Max / (srs.shahErrorThreshold * RdRMaxMax);
                    if (n0 < 0)
                        n0 = (Float) srs.shahMaxLightViewResolution;
                    /* Based on n0, a more realistic non-overlapping value can be calculated.
                     * This is done when the light is known.
                     */ 
#ifdef SSSDEBUG
                    std::string name = (diffusionMap == NULL) ? "None" : diffusionMap->toString();
                    Log(EDebug, "Set new realtime sss diffusion map (n0 = %i)", n0);
					Log(EDebug, "\t%s", name.c_str());
#endif
                }

                m_vplSampleOffset = 0; 

                oglRender(target);

				if (m_useSync)
					target.sync->init(); 
            } else {
				if (m_shaderManager == NULL || m_shaderManager->getScene() != m_context->scene) {
					if (m_shaderManager) {
						m_shaderManager->cleanup();
						m_framebuffer->cleanup();
					}
					m_shaderManager = new VPLShaderManager(m_context->scene, m_renderer);
					m_shaderManager->init();
					m_framebuffer->setFormat(GPUTexture::EFloat32RGB);
					m_framebuffer->setSize(size);
					m_framebuffer->setFilterType(GPUTexture::ENearest);
					m_framebuffer->setFrameBufferType(GPUTexture::EColorBuffer);
					m_framebuffer->setMipMapped(false);
					m_framebuffer->init();
				}

				m_shaderManager->setShadowMapResolution(m_context->shadowMapResolution);
				m_shaderManager->setClamping(m_context->clamping);
				m_shaderManager->setSinglePass(m_context->previewMethod == EOpenGLSinglePass);
				m_shaderManager->setDiffuseSources(m_context->diffuseSources);
				m_shaderManager->setDiffuseReceivers(m_context->diffuseReceivers);

				if (m_timer->getMilliseconds() > 1000) {
					Float count = m_vplsPerSecond / (Float) m_timer->getMilliseconds() * 1000;
					if (!m_motion)
						emit statusMessage(QString(formatString("%.1f VPLs/sec", count).c_str()));
					m_vplsPerSecond = 0;
					m_timer->reset();
				}

				if (m_vpls.empty()) {
					size_t oldOffset = m_vplSampleOffset;
					m_vplSampleOffset = generateVPLs(m_context->scene, m_random,
						m_vplSampleOffset, 1, m_context->pathLength, !m_motion, m_vpls);
					m_backgroundScaleFactor = m_vplSampleOffset - oldOffset;
				}

				VPL vpl = m_vpls.front();
				m_vpls.pop_front();

				oglRenderVPL(target, vpl);
				
				if (m_useSync)
					target.sync->init(); 
			}

			m_mutex->lock();
			m_vplsPerSecond++;
			m_vplCount++;

			if (m_minVPLs == 0) {
				if (m_timer->getMilliseconds() > 50) 
					m_minVPLs = m_vplCount;
			}

			if (m_vplCount >= m_minVPLs && m_minVPLs > 0)
				m_readyQueue.push_back(target);
			else
				m_recycleQueue.push_back(target);
			m_queueCV->signal();
			m_mutex->unlock();

			if (m_sleep)
				sleep(10);
		}
	} catch (std::exception &e) {
		m_started->set(true);
		Log(EWarn, "Caught an exception: %s", e.what());
		emit caughtException(e.what());
	}

	if (initializedGraphics) {
		if (m_shaderManager)
			m_shaderManager->cleanup();
		if (m_directShaderManager)
			m_directShaderManager->cleanup();

		m_accumProgram->cleanup();

		m_mutex->lock();
		while (!m_readyQueue.empty()) {
			PreviewQueueEntry &entry = m_readyQueue.back();
			if (entry.buffer) {
				entry.buffer->disassociate();
				entry.buffer->decRef();
			}
			if (entry.sync) 
				entry.sync->decRef();
			m_readyQueue.pop_back();
		}

		while (!m_recycleQueue.empty()) {
			PreviewQueueEntry &entry = m_recycleQueue.back();
			if (entry.buffer) {
				entry.buffer->disassociate();
				entry.buffer->decRef();
			}
			if (entry.sync) 
				entry.sync->decRef();
			m_recycleQueue.pop_back();
		}

		m_renderer->shutdown();
		m_device->shutdown();
		m_mutex->unlock();
	}

	MTS_AUTORELEASE_END()
}

void PreviewThread::oglRenderVPL(PreviewQueueEntry &target, const VPL &vpl) {
	const std::vector<std::pair<const TriMesh *, Transform> > meshes = m_shaderManager->getMeshes();

	m_shaderManager->setVPL(vpl);

	Point2 jitter(.5f, .5f);
	if (!m_motion && !m_context->showKDTree && m_accumBuffer != NULL)
		jitter -= Vector2(m_random->nextFloat(), m_random->nextFloat());

	m_mutex->lock();
	const ProjectiveCamera *camera = static_cast<const ProjectiveCamera *>
		(m_context->scene->getCamera());
	Transform projectionTransform = camera->getGLProjectionTransform(jitter);
	m_renderer->setCamera(projectionTransform.getMatrix(), m_camViewTransform.getMatrix());
	Transform clipToWorld = m_camViewTransform.inverse() 
		* Transform::scale(Vector(1, 1, -1)) * projectionTransform.inverse();

	target.vplSampleOffset = m_vplSampleOffset;
	Point camPos = m_camPos;
	m_mutex->unlock();

	m_framebuffer->activateTarget();
	m_framebuffer->clear();
	m_renderer->beginDrawingMeshes();
	for (size_t j=0; j<meshes.size(); j++) {
		const TriMesh *mesh = meshes[j].first;
		bool hasTransform = !meshes[j].second.isIdentity();
		m_shaderManager->configure(vpl, mesh->getBSDF(), 
			mesh->getLuminaire(), camPos, !mesh->hasVertexNormals());
		if (hasTransform)
			m_renderer->pushTransform(meshes[j].second);
		m_renderer->drawTriMesh(mesh);
		if (hasTransform)
			m_renderer->popTransform();
		m_shaderManager->unbind();
	}
	m_renderer->endDrawingMeshes();
    if (m_context->showNormals)
        oglRenderNormals(meshes); 

	m_shaderManager->drawBackground(clipToWorld, camPos,
		m_backgroundScaleFactor);
	m_framebuffer->releaseTarget();

	target.buffer->activateTarget();
	m_renderer->setDepthMask(false);
	m_renderer->setDepthTest(false);
	m_framebuffer->bind(0);
	if (m_accumBuffer == NULL) { 
		target.buffer->clear();
		m_renderer->blitTexture(m_framebuffer, true);
		m_framebuffer->blit(target.buffer, GPUTexture::EDepthBuffer);
	} else {
		m_accumBuffer->bind(1);
		m_accumProgram->bind();
		m_accumProgram->setParameter(m_accumProgramParam_source1, m_accumBuffer);
		m_accumProgram->setParameter(m_accumProgramParam_source2, m_framebuffer);
		m_renderer->blitQuad(true);
		m_accumProgram->unbind();
		m_accumBuffer->unbind();
		m_accumBuffer->blit(target.buffer, GPUTexture::EDepthBuffer);
	}

	m_renderer->setDepthMask(true);
	m_framebuffer->unbind();
	m_renderer->setDepthTest(true);
	target.buffer->releaseTarget();
	m_accumBuffer = target.buffer;

	static int i=0;
	if ((++i % 4) == 0 || m_motion) {
		/* Don't let the queue get too large -- this makes
		   the whole system unresponsive */
		m_renderer->finish();
	} else {
		if (m_useSync) {
			m_renderer->flush();
		} else {
			/* No sync objects available - we have to wait 
			   for everything to finish */
			m_renderer->finish();
		}
	}
}

void PreviewThread::oglRenderNormals(const std::vector<std::pair<const TriMesh *, Transform> > meshes) {
    Float scale = m_context->normalScaling;
    Spectrum normalColor;
    Normal n(0.0f);

    for (size_t j=0; j<meshes.size(); j++) {
        const TriMesh *mesh = meshes[j].first;
        if (mesh->hasVertexNormals()) {
            normalColor.fromLinearRGB(0.8f, 0.2f, 0.0f);
            m_renderer->setColor(normalColor);
            const Point *vertices = mesh->getVertexPositions();
            const Normal *normals = mesh->getVertexNormals();
            for (size_t i=0; i<mesh->getVertexCount(); ++i) {
                    m_renderer->drawLine( vertices[i], vertices[i] + scale * normals[i] );
            }
        } else {
            normalColor.fromLinearRGB(0.0f, 0.8f, 0.2f);
            m_renderer->setColor(normalColor);
            const Triangle *triangles = mesh->getTriangles();
            const Point *vertices = mesh->getVertexPositions();
            for (size_t i=0; i<mesh->getTriangleCount(); ++i) {
                const Triangle &tri = triangles[i];
                for (int j=0; j<3; ++j) {
                    const Point &v0 = vertices[tri.idx[j]];
                    const Point &v1 = vertices[tri.idx[(j+1)%3]];
                    const Point &v2 = vertices[tri.idx[(j+2)%3]];
                    Vector sideA(v1-v0), sideB(v2-v0);
                    if (i==0)
                        n = Normal(normalize(cross(sideA, sideB)));
                    const Point &center = (v0 + v1 + v2) / 3.0f;
                    m_renderer->drawLine( center, center + scale * n );
                }
            }
        }
    }
}

void PreviewThread::rtrtRenderVPL(PreviewQueueEntry &target, const VPL &vpl) {
	Float nearClip =  std::numeric_limits<Float>::infinity(),
		  farClip  = -std::numeric_limits<Float>::infinity();

	const int sampleCount = 200;
	const Float invSampleCount = 1.0f/sampleCount;
	Ray ray;
	ray.o = vpl.its.p;
	Intersection its;

	for (int i=1; i<=sampleCount; ++i) {
		Vector dir;
		Point2 seed(i*invSampleCount, radicalInverse(2, i)); // Hammersley seq.
		if (vpl.type == ESurfaceVPL || vpl.luminaire->getType() & Luminaire::EOnSurface)
			dir = vpl.its.shFrame.toWorld(squareToHemispherePSA(seed));
		else
			dir = squareToSphere(seed);
		ray.setDirection(dir);

		if (m_context->scene->rayIntersect(ray, its)) {
			nearClip = std::min(nearClip, its.t);
			farClip = std::max(farClip, its.t);
		}
	}

	Float minDist = nearClip + (farClip - nearClip) * m_context->clamping;

	if (nearClip >= farClip) {
		/* Unable to find any surface - just default values based on the scene size */
		nearClip = 1e-3 * m_context->scene->getBSphere().radius;
		farClip = 1e3 * m_context->scene->getBSphere().radius;
		minDist = 0;
	}

	Point2 jitter(.5f, .5f);
	if (!m_motion)
		jitter = Point2(m_random->nextFloat(), m_random->nextFloat());

	if (target.buffer->getBitmap() == NULL) 
		target.buffer->setBitmap(0, new Bitmap(target.buffer->getSize().x, 
			target.buffer->getSize().y, 96));

	m_mutex->lock();
	m_previewProc->configure(vpl, minDist, jitter, 
		m_accumBuffer ? m_accumBuffer->getBitmap() : NULL, 
		target.buffer->getBitmap(), 
		m_context->previewMethod == ERayTraceCoherent,
		m_context->diffuseSources,
		m_context->diffuseReceivers,
		m_backgroundScaleFactor);
	m_mutex->unlock();

	ref<Scheduler> sched = Scheduler::getInstance();
	sched->schedule(m_previewProc);
	sched->wait(m_previewProc);
	target.vplSampleOffset = m_vplSampleOffset;
	m_raysPerSecond += m_previewProc->getRayCount();
	m_accumBuffer = target.buffer;
}

static bool singleCheck = false;
static bool oglChecked = false;

void PreviewThread::oglErrorCheck() {
    GLenum errCode;
    const GLubyte *errString;

    if (singleCheck && oglChecked)
        return;

    oglChecked = true;
    if ((errCode = glGetError()) != GL_NO_ERROR) {
        errString = gluErrorString(errCode);
       fprintf (stderr, "OpenGL Error: %s\n", errString);
    } else {
       std::cerr << "No OpenGL errer" << std::endl;
    }
}

void PreviewThread::oglRender(PreviewQueueEntry &target) {
	const std::vector<std::pair<const Shape *, int> > shapes
        = m_directShaderManager->getShapes();

	Point2 jitter(.5f, .5f);
	//if (!m_motion && !m_context->showKDTree)
	//	jitter -= Vector2(m_random->nextFloat(), m_random->nextFloat());

	m_mutex->lock();
	const ProjectiveCamera *camera = static_cast<const ProjectiveCamera *>
		(m_context->scene->getCamera());
	Transform projectionTransform = camera->getGLProjectionTransform(jitter);
	Transform clipToWorld = m_camViewTransform.inverse() 
		* Transform::scale(Vector(1, 1, -1)) * projectionTransform.inverse();

	Point camPos = m_camPos;
	m_mutex->unlock();

    const SnowRenderSettings &srs = m_context->snowRenderSettings;
    const SnowMaterialManager &smm = m_context->snowMaterialManager;

    // expect for now only one spot light
    std::vector<Luminaire *> luminaires = m_context->scene->getLuminaires();
    if (luminaires.size() != 1)
        return;
    Luminaire *spot = luminaires[0];

    // Sample emmision informaton on the luminaire
    EmissionRecord eRec;
    spot->sampleEmissionArea(eRec, Point2());
    Float spotR, spotG, spotB;

    // Build light structure
    Transform spotTransform = spot->getLuminaireToWorld();
    m_currentSpot.aperture = spot->getAperture();
    m_currentSpot.pos = spotTransform(Point(0, 0, 0));
    m_currentSpot.dir =  spotTransform(Vector(0.0f, 0.0f, 1.0f));
    eRec.value.toLinearRGB(spotR, spotG, spotB);
    m_currentSpot.color =  Vector3(spotR, spotG, spotB);
    srs.shahSpecularColor.toLinearRGB(spotR, spotG, spotB);
    m_currentSpot.specularColor = Vector3(spotR, spotG, spotB);

    /* calculate light view resolution n and adapt FBO if needed. After
     * Shah et al. this is n = (n0 * W) / (2 * r_max). W is the lights
     * view frustum in world units. Limit the view size to a defined max.
     */
    Float W = 10.0f; // ToDo: Use the actual frustum
    int n = std::min( n0 * W / (2 * srs.shahRmax), (Float) srs.shahMaxLightViewResolution);
    if (n <= 0)
        n = (Float) srs.shahMaxLightViewResolution;
    if (n != fboSplatSize) {
        fboSplatSize = n;
        SLog(EDebug, "Realtime SSS: New light view resolution is %ix%i", n, n);
        fboLightView = new FrameBufferObject(2);
        fboLightView->init(fboSplatSize,fboSplatSize,intColFormRGBAF,
              wrapE,wrapE,filterL,filterL,FBO_DepthBufferType_TEXTURE,GL_NEAREST,GL_NEAREST,GL_CLAMP,GL_CLAMP);
        if (splatOrigins != NULL)
            delete [] splatOrigins;
        splatOrigins = new float[fboSplatSize * fboSplatSize * 3];
        if (splatColors != NULL)
            delete [] splatColors;
        splatColors = new float[fboSplatSize * fboSplatSize * 3];
    }

    bool backbufferSizeChanged =   (fboCumulSplatWidth != srs.shahBackbufferWidth)
                                || (fboCumulSplatHeight != srs.shahBackbufferHeight);
    if (backbufferSizeChanged) {
        fboCumulSplatWidth = srs.shahBackbufferWidth;
        fboCumulSplatHeight = srs.shahBackbufferHeight;

        fboView = new FrameBufferObject(1);
        fboViewExpand = new FrameBufferObject(1);
        fboCumulSplat = new FrameBufferObject(1);
        fboTmp = new FrameBufferObject(1);

        fboView->init(fboCumulSplatWidth,fboCumulSplatHeight,intColFormRGBAF,
            wrap,wrap,filter,filter,FBO_DepthBufferType_TEXTURE,GL_NEAREST,GL_NEAREST,GL_CLAMP,GL_CLAMP);
        fboViewExpand->init(fboCumulSplatWidth,fboCumulSplatHeight,intColFormRGBAF,
            wrap,wrap,filter,filter,FBO_DepthBufferType_TEXTURE,GL_NEAREST,GL_NEAREST,GL_CLAMP,GL_CLAMP);
        fboCumulSplat->init(fboCumulSplatWidth,fboCumulSplatHeight,intColFormRGBAF32,
            wrap,wrap,filterL,filterL,FBO_DepthBufferType_NONE,0,0,0,0);
        fboTmp->init(fboCumulSplatWidth,fboCumulSplatHeight,intColFormRGBF,
            wrap,wrap,filterL,filterL,FBO_DepthBufferType_NONE,0,0,0,0);
    }

    TranslucentShape ts;
    ts.albedoMap = albedoMap;
    ts.diffusionMap = diffusionMap;
    ts.splatRadius = srs.shahRmax;
     
	m_renderer->setCamera(projectionTransform.getMatrix(), m_camViewTransform.getMatrix());
	m_framebuffer->activateTarget();
    m_framebuffer->clear();

    const bool showSplatOrigins = srs.shahShowSplatOrigins;
    const bool showLight = srs.shahShowLight;

    // back up color buffer attriabutes
    glPushAttrib(GL_COLOR_BUFFER_BIT);
	for (size_t i=0; i<shapes.size(); i++) {
        int meshIdx = shapes[i].second;
        if (meshIdx == -1)
            continue;
        const TriMesh *mesh = m_directShaderManager->getMeshes()[meshIdx].first;
        if (smm.isMadeOfSnow( shapes[i].first )) {
            ts.mesh = mesh;

            calcSplatPositions(ts, camPos); // in light view

            // view matrix must have been set here
            calcVisiblePositions(ts); // in camera view

            combineSplats(ts); // in camera view

            /* give some debug output if requested */
            if (showSplatOrigins) {
                glPointSize(10.0);
                glBegin(GL_POINTS);
                std::vector<Splat>::iterator it;
                for(it=splats.begin(); it!=splats.end(); it++)
                {
                    glColor3fv(&(it->c.x));
                    glVertex3fv(&(it->o.x));
                }
                glEnd();
            }

            if (showLight) {
                glPointSize(10.0);
                glBegin(GL_POINTS);
                    glColor3fv(&(m_currentSpot.color.x));
                    glVertex3fv(&(m_currentSpot.pos.x));
                glEnd();
                glPointSize(1.0);
                glBegin(GL_LINES);
                    glVertex3fv(&(m_currentSpot.pos.x));
                    Point3 target = m_currentSpot.pos + m_currentSpot.dir;
                    glVertex3fv(&(target.x));
                glEnd();
            }

#ifdef SSSDEBUG 
            // save images of light view maps
            if (find(m_exportedMeshes.begin(), m_exportedMeshes.end(), mesh) == m_exportedMeshes.end()) {
                m_exportedMeshes.push_back(mesh);
                std::ostringstream name; name << "img-obj-" << mesh->getName();
                fboLightView->saveToDisk(0, name.str().append("-splat-o.exr"));
                fboLightView->saveToDisk(1, name.str().append("-splat-c.exr"));
            }

            // save images of light view maps
            if (find(m_camViewImages.begin(), m_camViewImages.end(), mesh) == m_camViewImages.end()) {
                m_camViewImages.push_back(mesh);
                std::ostringstream name; name << "img-obj-" << mesh->getName() << "-camView.exr";
                fboView->saveToDisk(0, name.str());
            }

            // save images of light view maps
            if (find(m_expansionImages.begin(), m_expansionImages.end(), mesh) == m_expansionImages.end()) {
                m_expansionImages.push_back(mesh);
                std::ostringstream name; name << "img-obj-" << mesh->getName() << "-expansion.exr";
                fboViewExpand->saveToDisk(0, name.str());
            }

            // save images of light view maps
            if (find(m_expansionImages.begin(), m_expansionImages.end(), mesh) == m_expansionImages.end()) {
                m_expansionImages.push_back(mesh);
                std::ostringstream name; name << "img-obj-" << mesh->getName() << "-cumulated.exr";
                fboViewExpand->saveToDisk(0, name.str());
            }

            // save images of light view maps
            if (find(m_finalImages.begin(), m_finalImages.end(), mesh) == m_finalImages.end()) {
                m_finalImages.push_back(mesh);
                Point3i size = m_framebuffer->getSize();
                ref<Bitmap> bitmap = new Bitmap(size.x, size.y, 128);
                m_framebuffer->download(bitmap);
                std::ostringstream name; name << "img-obj-" << mesh->getName() << "-final.exr";
                bitmap->save(Bitmap::EEXR, new FileStream(name.str(), FileStream::ETruncWrite));
            }
#endif
        } else { // mesh not made of snow
            m_renderer->beginDrawingMeshes();

            m_directShaderManager->configure(mesh->getBSDF(), 
                mesh->getLuminaire(), m_currentSpot, camPos, !mesh->hasVertexNormals());
            m_renderer->drawTriMesh(mesh);
            m_directShaderManager->unbind();
            m_renderer->endDrawingMeshes();
        }
    }

    // restore color data
    glPopAttrib();
	//m_shaderManager->drawBackground(clipToWorld, camPos);
	m_framebuffer->releaseTarget();

	target.buffer->activateTarget();
	m_renderer->setDepthMask(false);
	m_renderer->setDepthTest(false);
	m_framebuffer->bind(0);
	if (m_accumBuffer == NULL) { 
		target.buffer->clear();
		m_renderer->blitTexture(m_framebuffer, true);
	} else {
		m_accumBuffer->bind(1);
		m_accumProgram->bind();
		m_accumProgram->setParameter(m_accumProgramParam_source1, m_accumBuffer);
		m_accumProgram->setParameter(m_accumProgramParam_source2, m_framebuffer);
		m_renderer->blitQuad(true);
		m_accumProgram->unbind();
		m_accumBuffer->unbind();
	}
	m_framebuffer->unbind();
	m_renderer->setDepthMask(true);
	m_renderer->setDepthTest(true);
	target.buffer->releaseTarget();
	m_accumBuffer = target.buffer;

	static int i=0;
	if ((++i % 4) == 0 || m_motion) {
		/* Don't let the queue get too large -- this makes
		   the whole system unresponsive */
		m_renderer->finish();
	} else {
		if (m_useSync) {
			m_renderer->flush();
		} else {
			/* No sync objects available - we have to wait 
			   for everything to finish */
			m_renderer->finish();
		}
	}
}

void PreviewThread::calcSplatPositions(const TranslucentShape &ts, Point &camPos) {
	const std::vector<std::pair<const TriMesh *, Transform> > meshes
        = m_directShaderManager->getMeshes();

    // Setup spot light view space.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); // 1
    glLoadIdentity();
    gluPerspective(m_currentSpot.aperture, 1.0f, 0.01f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); // 2
    glLoadIdentity();
    gluLookAt(m_currentSpot.pos.x, m_currentSpot.pos.y, m_currentSpot.pos.z,
        m_currentSpot.pos.x + m_currentSpot.dir.x,
        m_currentSpot.pos.y + m_currentSpot.dir.y,
        m_currentSpot.pos.z + m_currentSpot.dir.z,
        0.0f,1.0f,0.0f);

    fboLightView->saveAndSetViewPort();

    GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
    fboLightView->enableRenderToColorAndDepth_MRT(2, drawBuffers);

    // Mark places where no objects are with 999.0
    glClearColor(999.0f, 999.0f, 999.0f, 999.0f);
    glColor3f(0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    // WiscombeWarren-Albedo will be calculated in the shader
    if (m_context->snowRenderSettings.shahAlbedoMapType != SnowRenderSettings::EWiscombeWarrenAlbedo) {
        GPUProgram *lightViewProgram = m_directShaderManager->m_lightViewProgram;
        lightViewProgram->bind();
        ts.albedoMap->bind(0);
        lightViewProgram->setParameter(m_directShaderManager->param_lightPos, m_currentSpot.pos);
        lightViewProgram->setParameter(m_directShaderManager->param_lightColor, m_currentSpot.color);
        lightViewProgram->setParameter(m_directShaderManager->param_lightDir, m_currentSpot.dir);
        lightViewProgram->setParameter(m_directShaderManager->param_lightAperture, degToRad(m_currentSpot.aperture * 0.5f));
        lightViewProgram->setParameter(m_directShaderManager->param_lightAlbedoTex, ts.albedoMap.get());

        /* It is currently  not necessary to translate trimeshes around. */
        //glMatrixMode(GL_TEXTURE);
        //glPushMatrix();
        //glLoadIdentity();
        //meshes[j]->getPosition(top);
        //glTranslatef(top.x,top.y,top.z);
        //glMatrixMode(GL_MODELVIEW);
        //glPushMatrix();
        //glTranslatef(top.x,top.y,top.z);

        // render the current translucent object
        m_renderer->beginDrawingMeshes();

        // needs tex coord and normal
        m_renderer->drawTriMesh(ts.mesh);

        m_renderer->endDrawingMeshes();

        //glPopMatrix();
        //glMatrixMode(GL_TEXTURE);
        //glPopMatrix();
        //glMatrixMode(GL_MODELVIEW);

        // unbind light view program and clean up
        lightViewProgram->unbind();
        ts.albedoMap->unbind();
    } else {
        ref<GPUProgram> lightViewWWProgram = m_directShaderManager->m_lightViewWWProgram;
        lightViewWWProgram->bind();
        lightViewWWProgram->setParameter(m_directShaderManager->param_lightWWCamPos, camPos);
        lightViewWWProgram->setParameter(m_directShaderManager->param_lightWWPos, m_currentSpot.pos);
        lightViewWWProgram->setParameter(m_directShaderManager->param_lightWWColor, m_currentSpot.color);
        lightViewWWProgram->setParameter(m_directShaderManager->param_lightWWDir, m_currentSpot.dir);
        lightViewWWProgram->setParameter(m_directShaderManager->param_lightWWAperture, degToRad(m_currentSpot.aperture * 0.5f));
        int texOffset = 0;
        m_directShaderManager->m_wiscombeWarrenShader->bind(lightViewWWProgram.get(),
            m_directShaderManager->m_wiscombeWarrenParams, texOffset);

        m_renderer->beginDrawingMeshes();
        // needs tex coord and normal
        m_renderer->drawTriMesh(ts.mesh);
        m_renderer->endDrawingMeshes();

        lightViewWWProgram->unbind();
    }

    fboLightView->disableRenderToColorDepth();

    //render occluders
    fboLightView->enableRenderToColorAndDepth(0);
    glDisable(GL_TEXTURE_2D);
    glColor3f(999.0f, 999.0f, 999.0f);

    for (size_t i=0; i<meshes.size(); i++) {
		const TriMesh *mesh = meshes[i].first;
        // don't render the current object as an ocluder
        if (mesh == ts.mesh)
            continue;

        //glMatrixMode(GL_MODELVIEW);
        //glPushMatrix();
        //(*ito)->getPosition(top);
        //glTranslatef(top.x,top.y,top.z);

        // no need for text coords and normal
        m_renderer->beginDrawingMeshes();
		bool hasTransform = !meshes[i].second.isIdentity();
		if (hasTransform)
			m_renderer->pushTransform(meshes[i].second);
		m_renderer->drawTriMesh(mesh);
		if (hasTransform)
			m_renderer->popTransform();
        m_renderer->endDrawingMeshes();

        //glPopMatrix();
    }

    glPopMatrix(); // 2
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); // 1
    glMatrixMode(GL_MODELVIEW);
    
    fboLightView->restoreViewPort();
    fboLightView->disableRenderToColorDepth();

    /* compute splat origin and intensity */
    glEnable(GL_TEXTURE_2D);
    fboLightView->bindColorTexture(0);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, splatOrigins);
    fboLightView->bindColorTexture(1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, splatColors);
    glDisable(GL_TEXTURE_2D);

    splats.clear();
    Vector3 o;
    Splat s;
    for(int i=0; i < fboSplatSize*fboSplatSize*3; i+=3) {
        if(   (splatOrigins[i] < 990.0f && splatOrigins[i+1] < 990.0f && splatOrigins[i+2] < 990.0f)
           && (splatColors[i] + splatColors[i+1] + splatColors[i+2]) > 0.0f )
        {
            s.c[0] = splatColors[i+0]; s.c[1] = splatColors[i+1]; s.c[2] = splatColors[i+2];
            // origin are in world space
            s.o[0] = splatOrigins[i+0]; s.o[1] = splatOrigins[i+1]; s.o[2] = splatOrigins[i+2];
            splats.push_back(s);
        }
    }
}

void PreviewThread::calcVisiblePositions(const TranslucentShape &ts) {
	const std::vector<std::pair<const TriMesh *, Transform> > meshes
        = m_directShaderManager->getMeshes();

    // Vector3 top;

    /* render surface data from view point (pixel position on
     * the current translucent object rendered */
    fboView->enableRenderToColorAndDepth(0);
    fboView->saveAndSetViewPort();
    // alpha=0.0 pixel is not on the current object (occluder or void)
    glClearColor(999.0f,999.0f,999.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    GPUProgram *cameraViewProgram = m_directShaderManager->m_cameraViewProgram;
    cameraViewProgram->bind();

    //render the current translucent object.
    //glMatrixMode(GL_TEXTURE);//model matrix
    //glPushMatrix();
    //glLoadIdentity();
    //curObj->getPosition(top);
    //glTranslatef(top.x,top.y,top.z);
    //glMatrixMode(GL_MODELVIEW);
    //glPushMatrix();
    //glTranslatef(top.x,top.y,top.z);

    // we need tex coords, but no normals
    m_renderer->beginDrawingMeshes();
    //bool hasTransform = !meshes[j].second.isIdentity();
    //if (hasTransform)
    //    m_renderer->pushTransform(meshes[j].second);
    m_renderer->drawTriMesh(ts.mesh);
    //if (hasTransform)
    //    m_renderer->popTransform();
    m_renderer->endDrawingMeshes();

    //glPopMatrix();
    //glMatrixMode(GL_TEXTURE);
    //glPopMatrix();
    //glMatrixMode(GL_MODELVIEW);
    cameraViewProgram->unbind();


    glDisable(GL_TEXTURE_2D);
    glColor4f(999.0f,999.0f,999.0f,0.0f);
    for(std::vector<std::pair<const TriMesh*, Transform> >::const_iterator it=meshes.begin();
            it!=meshes.end(); ++it) {
        //we do not render the current translucent object as an ocluder
        if((*it).first == ts.mesh)
            continue;
    
        //glPushMatrix();
        //(*ito)->getPosition(top);
        //glTranslatef(top.x,top.y,top.z);

        // we need tex coords, but no normals
        m_renderer->beginDrawingMeshes();
        m_renderer->drawTriMesh((*it).first);
        m_renderer->endDrawingMeshes();

        //glPopMatrix();
    }

    fboView->restoreViewPort();
    fboView->disableRenderToColorDepth();

    /* Silhouette expanding extension (by Sébastien Hillaire) */
    if (!(m_context->snowRenderSettings.shahExpandSilhouette))
        return;

    /* s expansion */
    float sOffset = 1.0f / fboView->getWidth();

    //glClientActiveTexture(GL_TEXTURE0);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    fboView->bindColorTexture(0);
    //expand view subsurface scattering data on s axis
    fboViewExpand->enableRenderToColorAndDepth(0);
    fboViewExpand->saveAndSetViewPort();

    GPUProgram *expansionProgram = m_directShaderManager->m_expandSilhouetteProgram;
    expansionProgram->bind();
    // ToDo: Wrap FBO implementation in sub class of GLProgram
    //expansionProgram->setParameter(m_directShaderManager->param_expandViewTex, fboView);
    glUniform1i(m_directShaderManager->param_expandViewTex, 0);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // glOrtho: (left, right, bottom, top, znear, zfar)
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
        glTexCoord4f(0.0f, 0.0f, sOffset, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord4f(1.0f, 0.0f, sOffset, 0.0f);
        glVertex2f(1.0f, 0.0f);
        glTexCoord4f(1.0f, 1.0f, sOffset, 0.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord4f(0.0f, 1.0f, sOffset, 0.0f);
        glVertex2f(0.0f, 1.0f);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    expansionProgram->unbind();
    fboViewExpand->restoreViewPort();
    fboViewExpand->disableRenderToColorDepth();

    /* t expansion */
    float tOffset = 1.0f / fboView->getHeight();

    fboViewExpand->bindColorTexture(0);
    //expand view subsurface scattering data on t axis
    fboView->enableRenderToColorAndDepth(0);
    fboView->saveAndSetViewPort();

    expansionProgram->bind();
    // ToDo: Wrap FBO implementation in sub class of GLProgram
    //expansionProgram->setParameter(m_directShaderManager->param_expandViewTex, fboViewExpand);
    glUniform1i(m_directShaderManager->param_expandViewTex, 0);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0,1.0,0.0,1.0,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
        glTexCoord4f(0.0f, 0.0f, 0.0f, tOffset);
        glVertex2f(0.0f, 0.0f);
        glTexCoord4f(1.0f, 0.0f, 0.0f, tOffset);
        glVertex2f(1.0f, 0.0f);
        glTexCoord4f(1.0f, 1.0f, 0.0f, tOffset);
        glVertex2f(1.0f, 1.0f);
        glTexCoord4f(0.0f, 1.0f, 0.0f, tOffset);
        glVertex2f(0.0f, 1.0f);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    expansionProgram->unbind();
    fboView->restoreViewPort();
    fboView->disableRenderToColorDepth();
}

void PreviewThread::combineSplats(const TranslucentShape &ts) {
    //Vector3 top;

    //splatting
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    // Bind camera view image to texture unit 0.
    fboView->bindColorTexture(0);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    /* For SSS configurations per object, each mesh would need to
     * its own diffusion map. Bind the current one to texture unit 1. */
    ts.diffusionMap->bind(1);

    glActiveTexture(GL_TEXTURE0);
    // No need for depth testing and depth buffer writing here 
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    /* Blending is needed for combining the different splat
     * contributions. Use additive blending. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE);

    // prepare output buffer and save/set viewport
    fboCumulSplat->enableRenderToColorAndDepth(0);
    fboCumulSplat->saveAndSetViewPort();
    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    GPUProgram *renderSplatsProgram = m_directShaderManager->m_renderSplatsProgram;
    renderSplatsProgram->bind();
    // ToDo: Wrap FBO implementation in sub class of GLProgram
    //renderSplatsProgram->setParameter(m_directShaderManager->param_renderSplatsViewSurfacePos, fboView);
    glUniform1i(m_directShaderManager->param_renderSplatsViewSurfacePos, 0);
    //renderSplatsProgram->setParameter(m_directShaderManager->param_renderSplatsTranslucencyTex, ts.diffusionMap.get());
    glUniform1i(m_directShaderManager->param_renderSplatsTranslucencyTex, 1);
    renderSplatsProgram->setParameter(m_directShaderManager->param_renderSplatsBillboardRadius, ts.splatRadius);
    const int attrib_bbOffset = m_directShaderManager->attrib_renderSplatsBillboardOffset;
    
    /* Accumulate each splat in the cumulation FBO. A single
     * splat is rendered as GL_QUAD. It checks for the contribution
     * at each corner and prints it to screen. Additive alpha
     * blending is used to add contributions up. */
    glBegin(GL_QUADS);
    std::vector<Splat>::iterator it;
    for(it=splats.begin(); it!=splats.end(); it++)
    {
        // push ponter to splat color as tex coord
        glTexCoord3fv(&(it->c.x));
    
        glVertexAttrib2fARB(attrib_bbOffset,-1.0f,-1.0f);
        glVertex3fv(&(it->o.x));
        
        glVertexAttrib2fARB(attrib_bbOffset, 1.0f,-1.0f);
        glVertex3fv(&(it->o.x));
        
        glVertexAttrib2fARB(attrib_bbOffset, 1.0f, 1.0f);
        glVertex3fv(&(it->o.x));
        
        glVertexAttrib2fARB(attrib_bbOffset,-1.0f, 1.0f);
        glVertex3fv(&(it->o.x));
    }   
    glEnd();

    renderSplatsProgram->unbind();
    fboCumulSplat->restoreViewPort();
    fboCumulSplat->disableRenderToColorDepth();
    ts.diffusionMap->unbind();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    /* Add contribution to view space by rendering the
     * 3D object again, but with the cumul FBO projected on it */
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    fboCumulSplat->bindColorTexture(0);

	m_framebuffer->activateTarget();
    //m_framebuffer->clear();
 
    // WiscombeWarren-Albedo will be calculated in the shader
    if (m_context->snowRenderSettings.shahAlbedoMapType != SnowRenderSettings::EWiscombeWarrenAlbedo) {
        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_2D);
        ts.albedoMap->bind(1);

        GPUProgram *finalContribProgram = m_directShaderManager->m_finalContributionProgram;
        finalContribProgram->bind();
        Float sampleScale = (Float) splats.size() / m_context->snowRenderSettings.shahWeight;
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribSampleScale, sampleScale);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribLightPos, m_currentSpot.pos);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribLightDir, m_currentSpot.dir);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribLightSpecColor, m_currentSpot.specularColor);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribLightAperture, degToRad(m_currentSpot.aperture * 0.5f));
        // ToDo: Wrap FBO implementation in sub class of GLProgram
        //finalContribProgram->setParameter(m_directShaderManager->param_finalContribSubSurf, fboCumulSplat);
        glUniform1i(m_directShaderManager->param_finalContribSubSurf, 0);
        //finalContribProgram->setParameter(m_directShaderManager->param_finalContribAlbedoTex, ts.albedoMap.get());
        glUniform1i(m_directShaderManager->param_finalContribAlbedoTex, 1);

        //glPushMatrix();
        //curObj->getPosition(top);
        //glTranslatef(top.x,top.y,top.z);
        // render the current translucent object
        m_renderer->beginDrawingMeshes();
        // needs tex coord and normal
        m_renderer->drawTriMesh(ts.mesh);
        m_renderer->endDrawingMeshes();

        //glPopMatrix();

        // clean up
        finalContribProgram->unbind();
        ts.albedoMap->unbind();
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
    } else {
        ref<GPUProgram> finalContribProgram = m_directShaderManager->m_finalContributionWWProgram;
        finalContribProgram->bind();
        Float sampleScale = (Float) splats.size() / m_context->snowRenderSettings.shahWeight;
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribWWSampleScale, sampleScale);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribWWLightPos, m_currentSpot.pos);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribWWLightDir, m_currentSpot.dir);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribWWLightSpecColor, m_currentSpot.specularColor);
        finalContribProgram->setParameter(m_directShaderManager->param_finalContribWWLightAperture, degToRad(m_currentSpot.aperture * 0.5f));

        glUniform1i(m_directShaderManager->param_finalContribWWSubSurf, 0);
        int texOffset = 0;
        m_directShaderManager->m_wiscombeWarrenShader->bind(finalContribProgram.get(),
            m_directShaderManager->m_finalContribWiscombeWarrenParams, texOffset);
        // ToDo: Wrap FBO implementation in sub class of GLProgram
        //finalContribProgram->setParameter(m_directShaderManager->param_finalContribSubSurf, fboCumulSplat);

        //glPushMatrix();
        //curObj->getPosition(top);
        //glTranslatef(top.x,top.y,top.z);
        // render the current translucent object
        m_renderer->beginDrawingMeshes();
        // needs tex coord and normal
        m_renderer->drawTriMesh(ts.mesh);
        m_renderer->endDrawingMeshes();

        //glPopMatrix();

        // clean up
        finalContribProgram->unbind();
    }

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
}

