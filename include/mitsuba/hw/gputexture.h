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

#if !defined(__GPUTEXTURE_H)
#define __GPUTEXTURE_H

#include <mitsuba/core/bitmap.h>
#include <set>

MTS_NAMESPACE_BEGIN

/** \brief A data structure for 1/2/3D and cube texture mapping. Also
 * has optional render-to-texture functionality.
 */
class MTS_EXPORT_HW GPUTexture : public Object {
public:
	/// Available texture types
	enum ETextureType {
		/**
		 * \brief 1-D texture, useful for the storage of pre-calculated functions
		 * in conjunction with pixel shaders. Needs 1D texture coordinates
		 */
		ETexture1D = 0,

		/// Default 2D texture format, needs 2D texture coordinates
		ETexture2D,

		/// 3D volume texture format, needs 3D texture coordinates
		ETexture3D,

		/// 3D cube map texture format, needs 3D texture coordinates
		ETextureCubeMap
	};

	/** \brief If the texture type is set to EFrameBuffer, the 
	 * configuration must be one of the following constants */
	enum EFrameBufferType {
		/// This is not a framebuffer
		ENone = 0x00,

		/**
		 * \brief Color framebuffer (\a including an internal depth 
		 * buffer that cannot be accessed as a texture)
		 */
		EColorBuffer = 0x01,

		/// Depth-only framebuffer (e.g. for shadow mapping)
		EDepthBuffer = 0x02,

		/// Color and texture framebuffer (both exposed as textures)
		EColorAndDepthBuffer = 0x03
	};

	/// A texture has one more slots into which bitmaps can be placed
	enum ETexturePosition {
		/// Default slot for 1,2,3D textures
		EDefaultPosition = 0,
		/// Cube map: +X plane, Right
		ECubeMapPositiveX = 0,
		/// Cube map: -X plane, Left
		ECubeMapNegativeX,
		/// Cube map: +Y plane, Top
		ECubeMapPositiveY,
		/// Cube map: -Y plane, Bottom
		ECubeMapNegativeY,
		/// Cube map: +Z plane, Front
		ECubeMapPositiveZ,
		/// Cube map: -Z plane, Back
		ECubeMapNegativeZ,
		ELastPosition
	};

	/** \brief Texture wrapping mode when texture coordinates
	 * exit the [0, 1] range
	 */
	enum EWrapType {
		/// Clamp the coordinates to [0, 1]
		EClamp = 0,
		/// Similar to EClamp, but prevents mixing at the edges
		EClampToEdge,
		/// Similar to EClamp
		EClampToBorder,
		/// Modulo 1 operation (default)
		ERepeat,
		/// Mirror the coordinates at the edges
		EMirroredRepeat
	};

	/// The texture pixel format
	enum ETextureFormat {
		/// RGB color values (each 8 bit)
		ER8G8B8 = 0,
		/// RGBA color values (each 8 bit)
		ER8G8B8A8,
		/// 8-bit luminance
		EL8,
		/// 8-bit luminance + alpha
		EL8A8,
		/// Depth component
		EDepth,
		/// Floating point luminance (16 bit)
		EFloat16L,
		/// Floating point RGB (16 bit)
		EFloat16RGB,
		/// Floating point RGBA (16 bit)
		EFloat16RGBA,
		/// Floating point luminance (32 bit)
		EFloat32L,
		/// Floating point RGB (32 bit)
		EFloat32RGB,
		/// Floating point RGBA (32 bit)
		EFloat32RGBA
	};

	/** \brief The interpolation filter determines which texture
	 * pixels are considered when shading a fragment
	 */
	enum EFilterType {
		/// Use the color value of the closest pixel
		ENearest = 0,
		/// Use 4 surrounding pixels and weigh them 
		ELinear,
		/**
		 * Blend the color values of the closest matching
		 * mipmaps and use nearest filtering
		 */
		EMipMapNearest,
		/**
		 * Blend the color values of the closest matching
		 * mipmaps and use linear filtering (aka. bilinear)
		 */
		EMipMapLinear
	};

	/**
	 * \brief When this texture contains a depth buffer, the
	 * following modes control the read behavior of the
	 * associated texture unit. 'ENormal' means that texture
	 * values are returned as with any other texture, whereas
	 * 'ECompare' causes a depth comparison to take place
	 * (this is the default).  
	 */
	enum EDepthMode {
		ENormal,
		ECompare
	};

	/** \brief Construct a new texture.
	 *
	 * If bitmap is non-NULL, the texture type, format 
	 * will be automatically set
	 *
	 * \param name A human-readable name (for debugging)
	 * \param bitmap An bitmap to be put into the first
	 * 		slot. A NULL value will be ignored.
	 */
	GPUTexture(const std::string &name, Bitmap *bitmap);

	/// Set the texture name
	inline void setName(const std::string &name) { m_name = name; }

	/// Get the texture name
	inline const std::string &getName() const { return m_name; }

	/// Set the texture type
	inline void setType(ETextureType textureType) { m_type = textureType; }

	/// Return the texture type
	inline ETextureType getType() const { return m_type; }
	
	/// Set the framebuffer type (applies only if type==EFrameBuffer)
	void setFrameBufferType(EFrameBufferType frameBufferType);

	/// Return the framebuffer type (applies only if type==EFrameBuffer)
	inline EFrameBufferType getFrameBufferType() const { return m_fbType; }

	/// Set the texture format
	inline void setFormat(ETextureFormat textureFormat) { m_format = textureFormat; }

	/// Return the texture format
	inline ETextureFormat getFormat() const { return m_format; }

	/// Set the filter type
	inline void setFilterType(EFilterType filterType) { m_filterType = filterType; }

	/// Return the filter type
	inline EFilterType getFilterType() const { return m_filterType; }

	/// Set the wrap type
	inline void setWrapType(EWrapType wrapType) { m_wrapType = wrapType; }

	/// Return the wrap type
	inline EWrapType getWrapType() const { return m_wrapType; }

	/// Return the size in pixels
	inline Point3i getSize() const { return m_size; }

	/// Set the size in pixels
	inline void setSize(const Point3i &size) { m_size = size; }

	/// Get the maximal anisotropy
	inline Float getMaxAnisotropy() const { return m_maxAnisotropy; }

	/** \brief Set the maximal anisotropy.
	 * 
	 * A value of 1 will result in isotropic
	 * texture filtering. A value of 0 (default) will
	 * use the global max. anisotropy value
	 */
	inline void setMaxAnisotropy(Float maxAnisotropy) { m_maxAnisotropy = maxAnisotropy; }

	/// Return whether mipmapping is enabled
	inline bool isMipMapped() const { return m_mipmapped; }

	/// Define whether mipmapping is enabled
	inline void setMipMapped(bool mipMapped) { m_mipmapped = mipMapped; }

	/// Return the depth map read mode
	inline EDepthMode getDepthMode() const { return m_depthMode; }
	
	/// Set the depth map read mode
	inline void setDepthMode(EDepthMode mode) { m_depthMode = mode; }

	/// Store a bitmap in a bitmap slot
	void setBitmap(unsigned int slot, Bitmap *bitmap);

	/// Retrieve a bitmap from the given slot
	Bitmap *getBitmap(unsigned int slot = EDefaultPosition);

	/// Retrieve a bitmap from the given slot
	const Bitmap *getBitmap(unsigned int slot = EDefaultPosition) const;

	/// Return the number of stored bitmaps
	inline int getBitmapCount() const { return (int) m_bitmaps.size(); }

	/// Upload the texture
	virtual void init() = 0;
	
	/// Refresh (re-upload) the texture
	virtual void refresh() = 0;

	/**
	 * This following applies only when textures are
	 * shared between threads while doing multi-context
	 * rendering. In this case, disassociate() needs to 
	 * be called once by each thread that used this
	 * texture before finally freeing it using cleanup().
	 */
	void disassociate();

	/// Free the texture from GPU memory
	virtual void cleanup() = 0;
	
	/**
	 * \brief Bind the texture and enable texturing
	 *
	 * \param textureUnit
	 *     Specifies the unit to which this texture should be bound
	 * \param textureIndex
	 *     When this texture has multiple sub-textures (e.g.
	 *     a color and depth map in the case of a 
	 *     \ref EColorAndDepthBuffer texture), this parameter
	 *     specifies the one to be bound
	 */
	virtual void bind(int textureUnit = 0, int textureIndex = 0) const = 0;

	/// Unbind the texture and disable texturing
	virtual void unbind() const = 0;

	/**
	 * Download the texture (only for render target textures).
	 * When the 'bitmap' parameter is NULL, the destination is
	 * the internal bitmap given by getTexture(0)
	 */
	virtual void download(Bitmap *bitmap = NULL) = 0;

	/// Activate the render target
	virtual void activateTarget() = 0;

	/// Activate a certain face of a cube map as render target
	virtual void activateSide(int side) = 0;

	/// Restrict rendering to a sub-region of the texture
	virtual void setTargetRegion(const Point2i &offset, const Vector2i &size) = 0;

	/// Deactivate the render target
	virtual void releaseTarget() = 0;

	/// Return the texture units, to which this texture is currently bound
	inline const std::set<int> &getTextureUnits() const { return m_textureUnits.get(); }

	/// Set the number of samples (for multisample color render targets)
	inline void setSampleCount(int samples) { m_samples = samples; }
	
	/// Return the number of samples (for multisample color render targets)
	inline int getSampleCount() const { return m_samples; }

	/**
	 * \brief Blit a render buffer into another render buffer
	 *
	 * \param target
	 *     Specifies the target render buffer
	 * \param what
	 *     A bitwise-OR of the components in \ref EFrameBufferType to copy 
	 */
	virtual void blit(GPUTexture *target, int what) const = 0;

	/**
	 * \brief Blit a render buffer into another render buffer
	 *
	 * \param target
	 *     Specifies the target render buffer (or NULL for the framebuffer)
	 * \param what
	 *     A bitwise-OR of the components in \ref EFrameBufferType to copy 
	 * \param sourceOffset 
	 *     Offset in the source render buffer
	 * \param sourceOffset 
	 *     Size of the region to be copied from the source render buffer
	 * \param destOffset 
	 *     Offset in the destination render buffer
	 * \param destOffset 
	 *     Size of the region to be copied into the dest destination buffer
	 */
	virtual void blit(GPUTexture *target, int what, const Point2i &sourceOffset,
		const Vector2i &sourceSize, const Point2i &destOffset,
		const Vector2i &destSize) const = 0;

	/// Clear (assuming that this is a render buffer)
	virtual void clear() = 0;

	/// Assuming that this is a 2D RGB framebuffer, read a single pixel from the GPU
	virtual Spectrum getPixel(int x, int y) const = 0;

	/// Return a string representation
	std::string toString() const;

	MTS_DECLARE_CLASS()
protected:
	/// Virtual destructor
	virtual ~GPUTexture();
protected:
	std::string m_name;
	ETextureType m_type;
	ETextureFormat m_format;
	EFilterType m_filterType;
	EWrapType m_wrapType;
	EFrameBufferType m_fbType;
	EDepthMode m_depthMode;
    bool m_mipmapped;
	mutable PrimitiveThreadLocal<std::set<int> > m_textureUnits;
	Float m_maxAnisotropy;
	int m_samples;
	std::vector<Bitmap *> m_bitmaps;
	Point3i m_size;
};

MTS_NAMESPACE_END

#endif /* __GPUTEXTURE_H */
