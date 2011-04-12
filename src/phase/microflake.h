/*
    This file is part of Mitsuba, a physically based rendering system.

    Copyright (c) 2007-2010 by Wenzel Jakob and others.

    Mitsuba is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Mitsuba is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(__MICROFLAKE_SIGMA_T_H)
#define __MICROFLAKE_SIGMA_T_H

MTS_NAMESPACE_BEGIN

#define SIGMA_T_RANGE_MIN 4e-08
#define SIGMA_T_RANGE_MAX 4
#define SIGMA_T_ELEMENTS  100
#define SIGMA_T_COEFFS    10

/**
 * Each row of this table table stores an expansion of \sigma_t in terms of
 * successive powers of sin(theta), where theta denotes the angle between the
 * fiber axis and the incident direction. The standard deviation of the
 * underlying distribution increases from row to row using a nonlinear 
 * mapping to capture the `interesting' parts more efficiently (see
 * \ref sigmaT_fiberGaussian())
 *
 * The implementation here has an average absolute error of 3.17534e-05.
 */
const double fiberGaussianCoeffs[SIGMA_T_ELEMENTS][SIGMA_T_COEFFS] = {
	{ 2.5037347588631811e-08,   6.3661860705430207e-01,   1.7827351092236654e-05,  -1.3015084365974872e-04,   5.2848718115683369e-04,  -1.2810825925271274e-03,   1.8970200528656278e-03,  -1.6818874867112754e-03,   8.1991128831759852e-04,  -1.6898472595983094e-04 },
	{ 4.0059757909494120e-07,   6.3660112034736627e-01,   2.8523763553955916e-04,  -2.0824136415171779e-03,   8.4557955473769653e-03,  -2.0497323170502568e-02,   3.0352323203715059e-02,  -2.6910202150020268e-02,   1.3118581833396092e-02,  -2.7037558688078889e-03 },
	{ 2.0280252492606374e-06,   6.3652534713591225e-01,   1.4440155345667449e-03,  -1.0542219099960448e-02,   4.2807465147063795e-02,  -1.0376769900017280e-01,   1.5365863698661997e-01,  -1.3623289910560743e-01,   6.6412820861387445e-02,  -1.3687764171763206e-02 },
	{ 6.4095612837167422e-06,   6.3632134209987312e-01,   4.5638021867375755e-03,  -3.3318618416970480e-02,   1.3529272946198034e-01,  -3.2795717240639988e-01,   4.8563717411593643e-01,  -4.3056323703422095e-01,   2.0989731056533856e-01,  -4.3260094199922605e-02 },
	{ 1.5648342979348966e-05,   6.3589118268124301e-01,   1.1142095183371836e-02,  -8.1344283251908678e-02,   3.3030451530680693e-01,  -8.0067669048537482e-01,   1.1856376321622975e+00,  -1.0511797779829521e+00,   5.1244460589555274e-01,  -1.0561546436373703e-01 },
	{ 3.2456085128154894e-05,   6.3510930296508339e-01,   2.3095905053374111e-02,  -1.6860353686243457e-01,   6.8460141516334261e-01,  -1.6594719054512552e+00,   2.4572952028929649e+00,  -2.1785971945564597e+00,   1.0620445192134866e+00,  -2.1888703273525323e-01 },
	{ 6.0167126588393707e-05,   6.3382310574343936e-01,   4.2746463734340878e-02,  -3.1200033264376259e-01,   1.2667238965446472e+00,  -3.0703332136446306e+00,   4.5462469549395337e+00,  -4.0304912157873787e+00,   1.9647737358992572e+00,  -4.0493097576113257e-01 },
	{ 1.0271798936965673e-04,   6.3185207235896912e-01,   7.2841433658423327e-02,  -5.3154994219562468e-01,   2.1578415962123927e+00,  -5.2298572591164429e+00,   7.7434431269682591e+00,  -6.8647060616517592e+00,   3.3462884896129026e+00,  -6.8963842484261306e-01 },
	{ 1.6470029799769362e-04,   6.2899006218207520e-01,   1.1649754982947513e-01,  -8.4988450803844273e-01,   3.4495702372051937e+00,  -8.3596831930942699e+00,   1.2376630927698898e+01,  -1.0971525201431632e+01,   5.3479906155741901e+00,  -1.1021346587513392e+00 },
	{ 2.5137803488298627e-04,   6.2500606147238247e-01,   1.7718177426831794e-01,  -1.2920889761092269e+00,   5.2432395119678290e+00,  -1.2704607676773321e+01,   1.8807441832920404e+01,  -1.6671004867676174e+01,   8.1256944770059363e+00,  -1.6744986823954235e+00 },
	{ 3.6872616081699621e-04,   6.1964591982446715e-01,   2.5866811590267780e-01,  -1.8853305942420775e+00,   7.6482657005151964e+00,  -1.8528457768656949e+01,   2.7425102889242680e+01,  -2.4307290410872838e+01,   1.1846818910639968e+01,  -2.4411789485290569e+00 },
	{ 5.2349862800175114e-04,   6.1263499404287958e-01,   3.6497063499826332e-01,  -2.6582767698899232e+00,   1.0779563479728154e+01,  -2.6107454585153732e+01,   3.8636223721863416e+01,  -3.4239300942256932e+01,   1.6685744794289462e+01,  -3.4380192976765613e+00 },
	{ 7.2329950522299295e-04,   6.0368178643211867e-01,   5.0025412688052207e-01,  -3.6403297720590615e+00,   1.4754180128773555e+01,  -3.5721657662357529e+01,   5.2851724760172829e+01,  -4.6828882541236908e+01,   2.2817946206523288e+01,  -4.7010346704690136e+00 },
	{ 9.7670257197230662e-04,   5.9248205150475330e-01,   6.6872566283534951e-01,  -4.8606744453900745e+00,   1.9687050003096772e+01,  -4.7644083917726675e+01,   7.0470126528386800e+01,  -6.2425612195045687e+01,   3.0412444564235102e+01,  -6.2648341623841617e+00 },
	{ 1.2933886441652406e-03,   5.7872490933544274e-01,   8.7448129174092060e-01,  -6.3469458498602336e+00,   2.5685051344618500e+01,  -6.2125392213743567e+01,   9.1853842979496221e+01,  -8.1345045652439694e+01,   3.9620905682830426e+01,  -8.1603211205835038e+00 },
	{ 1.6843402077874820e-03,   5.6210008633096731e-01,   1.1213197937836537e+00,  -8.1236497996889767e+00,   3.2840234018292733e+01,  -7.9377194678427145e+01,   1.1730450084337895e+02,  -1.0384705840605693e+02,   5.0567237033195283e+01,  -1.0412585838969036e+01 },
	{ 2.1621642818823233e-03,   5.4230609414093234e-01,   1.4124827310886330e+00,  -1.0209626432549257e+01,   4.1217471426223256e+01,  -9.9537968722748928e+01,   1.4700722220465104e+02,  -1.3008245776206479e+02,   6.3319879928942100e+01,  -1.3034893112532046e+01 },
	{ 2.7414845058320253e-03,   5.1906261307565105e-01,   1.7503339483908267e+00,  -1.2615517554806075e+01,   5.0844653960418441e+01,  -1.2265168120733540e+02,   1.8100314424548401e+02,  -1.6007238930855954e+02,   7.7883540482666831e+01,  -1.6027320715765825e+01 },
	{ 3.4399318918636639e-03,   4.9211465387090225e-01,   2.1359741840179609e+00,  -1.5339433938805108e+01,   6.1690387809545257e+01,  -1.4860442152337600e+02,   2.1908396187004291e+02,  -1.9360595630097259e+02,   9.4145485241585277e+01,  -1.9364996411770477e+01 },
	{ 4.2793046882470074e-03,   4.6125395191193763e-01,   2.5684830882320924e+00,  -1.8359912269903546e+01,   7.3632029150638630e+01,  -1.7704202186731663e+02,   2.6066533651762620e+02,  -2.3012529566340095e+02,   1.1181863782463552e+02,  -2.2986248930483882e+01 },
	{ 5.2852090222032097e-03,   4.2635627720729807e-01,   3.0442423654828543e+00,  -2.1630477435836340e+01,   8.6432271620200197e+01,  -2.0731144083331878e+02,   3.0469903227422901e+02,  -2.6864794753812635e+02,   1.3040432511362485e+02,  -2.6785122445907291e+01 },
	{ 6.4846390229581167e-03,   3.8744041600645113e-01,   3.5567082585622636e+00,  -2.5081508858791750e+01,   9.9757219307623814e+01,  -2.3852358684730211e+02,   3.4978714385790249e+02,  -3.0788227382415596e+02,   1.4925370456679053e+02,  -3.0624825656827412e+01 },
	{ 7.9031604369322068e-03,   3.4476326646197070e-01,   4.0953742753670364e+00,  -2.8614458527113896e+01,   1.1315651349144355e+02,  -2.6950799445251209e+02,   3.9411365819250113e+02,  -3.4616256623532286e+02,   1.6753318385760520e+02,  -3.4329892305929590e+01 },
	{ 9.5637414989571158e-03,   2.9891054885236912e-01,   4.6443809420073254e+00,  -3.2092399498624331e+01,   1.2602932275986439e+02,  -2.9874343352909591e+02,   4.3536321094438404e+02,  -3.8139704247261921e+02,   1.8420896219870235e+02,  -3.7685014731577979e+01 },
	{ 1.1486341108385352e-02,   2.5085629226590367e-01,   5.1814039353357577e+00,  -3.5331072148963763e+01,   1.3758600663377985e+02,  -3.2426285781884656e+02,   4.7058054787953040e+02,  -4.1094590847995823e+02,   1.9798984524766331e+02,  -4.0423874060840575e+01 },
	{ 1.3688331108499648e-02,   2.0185738938760295e-01,   5.6804606225381189e+00,  -3.8125268875733710e+01,   1.4696980305823266e+02,  -3.4396671914102319e+02,   4.9664324155754014e+02,  -4.3203638570659280e+02,   2.0752343939984439e+02,  -4.2267713842425479e+01 },
	{ 1.6184227239694308e-02,   1.5345823591227242e-01,   6.1119782196119630e+00,  -4.0249903931322180e+01,   1.5326401644052635e+02,  -3.5565042961554434e+02,   5.1032205343298892e+02,  -4.4183728527562516e+02,   2.1144408009392555e+02,  -4.2937785167319149e+01 },
	{ 1.8985816347600583e-02,   1.0737324130591908e-01,   6.4456745027165585e+00,  -4.1485979438363131e+01,   1.5561111930562549e+02,  -3.5731744543875107e+02,   5.0877258965185274e+02,  -4.3791497548811390e+02,   2.0860323039842808e+02,  -4.2204249006292926e+01 },
	{ 2.2103307834590302e-02,   6.5315550160654545e-02,   6.6543245599932401e+00,  -4.1652199318421850e+01,   1.5334623388000242e+02,  -3.4749681264899391e+02,   4.8997896810247835e+02,  -4.1859326773601038e+02,   1.9822564639309428e+02,  -3.9914040985628247e+01 },
	{ 2.5545627624255097e-02,   2.8910998107536945e-02,   6.7154591783420745e+00,  -4.0618917924898092e+01,   1.4606290439459454e+02,  -3.2543507026888392e+02,   4.5309911680833284e+02,  -3.8332203400710205e+02,   1.8012209130751432e+02,  -3.6041800421135434e+01 },
	{ 2.9322567133884130e-02,  -5.7848128586956982e-04,   6.6169952060026054e+00,  -3.8353439262067951e+01,   1.3379892723850037e+02,  -2.9152784105848627e+02,   3.9905357140199021e+02,  -3.3314206958034538e+02,   1.5488436409611415e+02,  -3.0723129294247755e+01 },
	{ 3.3445386512875028e-02,  -2.2246309887429816e-02,   6.3558799243823323e+00,  -3.4902480875424679e+01,   1.1694581950467341e+02,  -2.4708071930404594e+02,   3.3017518912771015e+02,  -2.7040021327619070e+02,   1.2376692318030155e+02,  -2.4235580997291095e+01 },
	{ 3.7929679841580549e-02,  -3.5932119064325585e-02,   5.9461254486949908e+00,  -3.0457694519905928e+01,   9.6521853209743824e+01,  -1.9495341514202153e+02,   2.5110041031038759e+02,  -1.9946123317581987e+02,   8.8988099033450908e+01,  -1.7050263312749763e+01 },
	{ 4.2795601672369385e-02,  -4.2098631408150400e-02,   5.4144421945551802e+00,  -2.5309259206875566e+01,   7.3943228292167134e+01,  -1.3894571658331506e+02,   1.6781656864374054e+02,  -1.2585426608991702e+02,   5.3320229288468454e+01,  -9.7502196845601361e+00 },
	{ 4.8068884486939398e-02,  -4.1871421597432290e-02,   4.7996956744822352e+00,  -1.9833388599886689e+01,   5.0943788866942896e+01,  -8.3542741827361397e+01,   8.7208601298231542e+01,  -5.5812018778307447e+01,   1.9839439199389972e+01,  -2.9740928416947554e+00 },
	{ 5.3780077827442485e-02,  -3.6796206499391304e-02,   4.1463577549930957e+00,  -1.4429895632283120e+01,   2.9283439309683047e+01,  -3.3152601924612441e+01,   1.5884834315236958e+01,   4.7885561802165739e+00,  -8.5925104736712683e+00,   2.6900357012082452e+00 },
	{ 5.9964904075551946e-02,  -2.8866469866554834e-02,   3.5042321255175892e+00,  -9.5153338793677165e+00,   1.0697160703523366e+01,   8.0864111136576184e+00,  -4.0229095823679700e+01,   5.0905271514639935e+01,  -2.9623593857883179e+01,   6.7786993537486069e+00 },
	{ 6.6661008627535462e-02,  -1.9928360316207483e-02,   2.9145446668658970e+00,  -5.4007174294653595e+00,  -3.6362107295921078e+00,   3.7590479275597772e+01,  -7.7700016609916929e+01,   7.9801875581656077e+01,  -4.2047830540959581e+01,   9.0655636532034407e+00 },
	{ 7.3906932207307693e-02,  -1.1576737486988620e-02,   2.4080962732199467e+00,  -2.2800405499556291e+00,  -1.3134188619792837e+01,   5.4432189178139197e+01,  -9.5776837578258665e+01,   9.1274497014057943e+01,  -4.5953002646290201e+01,   9.6008559933134734e+00 },
	{ 8.1739795518797176e-02,  -4.8933071167569508e-03,   2.0003980866159381e+00,  -1.9677422188347293e-01,  -1.7922032714324757e+01,   5.9534867660569489e+01,  -9.6559908120214459e+01,   8.7698272775128316e+01,  -4.2695143595016589e+01,   8.6967457183222621e+00 },
	{ 9.0195346006688423e-02,  -4.2829424927592896e-04,   1.6921823622516925e+00,   9.4229895604706826e-01,  -1.8733372090377827e+01,   5.5337143400918194e+01,  -8.4385005146074036e+01,   7.3392984594058362e+01,  -3.4553630367273399e+01,   6.8501542358267500e+00 },
	{ 9.9305741900081290e-02,   1.8863175328434068e-03,   1.4708655195770923e+00,   1.3342198264882938e+00,  -1.6700405772181057e+01,   4.5141091248175599e+01,  -6.4700977522621542e+01,   5.3527981533489765e+01,  -2.4163373390562583e+01,   4.6210399064717649e+00 },
	{ 1.0910052889768161e-01,   2.6435856561144444e-03,   1.3116093364974688e+00,   1.2700813449279735e+00,  -1.3293959152477555e+01,   3.2899513398500631e+01,  -4.3634294568378209e+01,   3.3614464470683743e+01,  -1.4202345384386945e+01,   2.5537661871579189e+00 },
	{ 1.1960981516169969e-01,   2.3969047848775293e-03,   1.1935815450518881e+00,   9.7534210089987994e-01,  -9.5747825233650019e+00,   2.1288573613199560e+01,  -2.5113344026101458e+01,   1.7029400917394923e+01,  -6.2521115916324561e+00,   9.6066844521743633e-01 },
	{ 1.3086369889448535e-01,   1.7213644020392938e-03,   1.0979243426374019e+00,   6.3251897859009887e-01,  -6.3135199488740454e+00,   1.2045961176761921e+01,  -1.1401663565375202e+01,   5.4833890639265519e+00,  -1.0088767509500940e+00,  -4.0450833162253730e-02 },
	{ 1.4289381667983458e-01,   1.0261077263953311e-03,   1.0120124278733265e+00,   3.4748953748527356e-01,  -3.8676790002325561e+00,   5.7400156179733131e+00,  -2.8791713647910910e+00,  -1.0573215143358539e+00,   1.6982620546484668e+00,  -5.1138886009221096e-01 },
	{ 1.5573358642688839e-01,   4.8969682279409454e-04,   9.3015556410425049e-01,   1.5191000340934124e-01,  -2.2336267819050590e+00,   2.0021183401931921e+00,   1.4689217270611152e+00,  -3.8262808258941732e+00,   2.5963972420631762e+00,  -6.2171395889038195e-01 },
	{ 1.6941661031438571e-01,   2.0760196204205883e-04,   8.4936397416857901e-01,   5.1231615081320570e-02,  -1.3046563347473921e+00,   2.7370274541499384e-01,   2.8138136655779817e+00,  -4.0773028931475892e+00,   2.3672523715808893e+00,  -5.2131655700821966e-01 },
	{ 1.8397230685417876e-01,   1.3285293716691626e-04,   7.7029177606227250e-01,   1.8504161778537309e-02,  -8.4682677030491504e-01,  -2.6656775494620888e-01,   2.6403875003352368e+00,  -3.2886595187483181e+00,   1.7855073089822326e+00,  -3.7783939463486149e-01 },
	{ 1.9941631497199752e-01,   6.0082630864766173e-05,   6.9733334836982408e-01,   2.4914487812566222e-04,  -5.6648868589604717e-01,  -4.0106518688662618e-01,   2.0584432918349194e+00,  -2.3119016347500292e+00,   1.1772197195668923e+00,  -2.3765569767721217e-01 },
	{ 2.1572713447091724e-01,   3.4986717258433941e-05,   6.2905902774097910e-01,  -3.3406878690982467e-03,  -4.1214601929550554e-01,  -3.2940858349866176e-01,   1.4119192291809668e+00,  -1.4701320162648699e+00,   7.0646817487568114e-01,  -1.3640374095473362e-01 },
	{ 2.3282391203135863e-01,   2.8932017583027658e-05,   5.6571928541158201e-01,  -1.9535588649546298e-03,  -3.1674712727340193e-01,  -2.1887133687550886e-01,   8.9858247790834866e-01,  -8.7755146968629560e-01,   3.9957544247909027e-01,  -7.4240635272644795e-02 },
	{ 2.5054712632571263e-01,   3.2510716290179431e-05,   5.0719666515726658e-01,   8.4930911680203280e-04,  -2.5202862723335784e-01,  -1.2167753882022225e-01,   5.3774374643217016e-01,  -4.9710679393865576e-01,   2.1608309793009539e-01,  -3.9260940853978354e-02 },
	{ 2.6865798477639780e-01,   2.9870654548957987e-05,   4.5348605584193535e-01,   2.8556756968214358e-03,  -2.0238836353769329e-01,  -5.4170724183222774e-02,   3.0495225597860554e-01,  -2.6904825927704223e-01,   1.1333033277378490e-01,  -2.0836300342239156e-02 },
	{ 2.8685842956216845e-01,   3.1114778236585039e-05,   4.0433696615284020e-01,   4.0340474949402960e-03,  -1.6176264190016809e-01,  -1.6488929640217975e-02,   1.7101015123171237e-01,  -1.4808205902409100e-01,   6.3804586780406680e-02,  -1.2803683156448642e-02 },
	{ 3.0482787935614586e-01,   3.3384776219946843e-05,   3.5955678907787103e-01,   5.4553629408928828e-03,  -1.3362866028228382e-01,   1.7531394174511661e-02,   7.1098833423093311e-02,  -6.1780374330737686e-02,   2.8427708666640683e-02,  -6.7974770054206601e-03 },
	{ 3.2226155495895903e-01,   3.4155237169208874e-05,   3.1914317291171557e-01,   5.4072789322390236e-03,  -1.0620948645509998e-01,   2.4282179338115384e-02,   3.3248619322876038e-02,  -3.3968577819905477e-02,   2.0084912335732952e-02,  -5.9009344369087557e-03 },
	{ 3.3890221366950402e-01,   2.0546877221860882e-05,   2.8304455820945407e-01,   3.3146357809528126e-03,  -7.6129448053279702e-02,   3.8821998550702119e-03,   4.8730105821960024e-02,  -5.4542060603125719e-02,   3.4238536472457781e-02,  -9.3994808930801810e-03 },
	{ 3.5455646797597412e-01,   1.8362283423556391e-05,   2.5060249242221744e-01,   2.6322850935684983e-03,  -5.6888634119559356e-02,  -3.8989591939753154e-03,   5.1888015595977777e-02,  -6.3082321563342703e-02,   4.1296355962913367e-02,  -1.1231434904630078e-02 },
	{ 3.6909984701618909e-01,   1.6536847965653578e-05,   2.2172548357023913e-01,   2.2320545546108406e-03,  -4.3372454115342407e-02,  -5.9923842527496163e-03,   4.7562927758917795e-02,  -6.1368319249595515e-02,   4.1351291120918177e-02,  -1.1276020686864285e-02 },
	{ 3.8247080552158264e-01,   1.8131270623555906e-05,   1.9604837138786024e-01,   2.7118367889329420e-03,  -3.6936880845132691e-02,   2.8190811190142995e-03,   2.7670312196278246e-02,  -4.3342948090867139e-02,   3.2094472096105164e-02,  -9.1580671610245190e-03 },
	{ 3.9465957574837796e-01,   1.7465775465019817e-05,   1.7343430798564441e-01,   2.3569007502146633e-03,  -2.8181566828607174e-02,  -2.0189997085253708e-03,   3.2701899466701434e-02,  -4.9349836974215577e-02,   3.5375967266190855e-02,  -9.8081622037398120e-03 },
	{ 4.0569541980632351e-01,   1.9958635579797601e-05,   1.5340962792157598e-01,   3.0511333648064465e-03,  -2.6441728714871715e-02,   7.9630436275692773e-03,   1.4609918827773072e-02,  -3.3061443275073543e-02,   2.7135316435305867e-02,  -8.0017456665473219e-03 },
	{ 4.1563457636817253e-01,   5.5992007963467927e-06,   1.3610792333440713e-01,   9.8722518885097088e-04,  -1.3647461077653134e-02,  -1.4160927474222262e-02,   4.7536840113025391e-02,  -6.4077645143697737e-02,   4.2970151088411512e-02,  -1.1380402027270975e-02 },
	{ 4.2454957586533054e-01,   3.8795934456103964e-06,   1.2069543428840745e-01,   7.8677885090883137e-04,  -1.0064601633985149e-02,  -1.4513994588924106e-02,   4.5876404419686878e-02,  -6.1718381737591699e-02,   4.1230072974030918e-02,  -1.0876844232370786e-02 },
	{ 4.3252185574001067e-01,   4.3441559727241952e-06,   1.0710706026918615e-01,   9.5231785212490649e-04,  -8.8316895730713441e-03,  -1.0535516626987373e-02,   3.7417189047118882e-02,  -5.2574371995433467e-02,   3.5828778641189274e-02,  -9.5506345173816953e-03 },
	{ 4.3963559755258452e-01,   6.1788380136817977e-06,   9.5127300113588831e-02,   1.3596035990985911e-03,  -8.9569400738582772e-03,  -5.2640362991951406e-03,   2.8439507819712162e-02,  -4.4066917123927851e-02,   3.1317015257627645e-02,  -8.5316145878095995e-03 },
	{ 4.4597386045617482e-01,   1.5105019519623397e-06,   8.4755811411383775e-02,   1.1432714251213838e-04,  -1.4451622237174888e-03,  -2.2321002003081958e-02,   5.6447476590619772e-02,  -7.1486492555777659e-02,   4.5814918963515083e-02,  -1.1734050668792406e-02 },
	{ 4.5161576857403135e-01,   6.1715483772317725e-06,   7.5392486563373495e-02,   1.4194563996738907e-03,  -7.3998385532831890e-03,  -1.0040536872111261e-03,   1.8471973291525501e-02,  -3.2531490722249146e-02,   2.4322443325218046e-02,  -6.8145008419833175e-03 },
	{ 4.5663544346694185e-01,   5.3526315193508367e-06,   6.7271514795251619e-02,   1.2473513968416228e-03,  -5.8618022317205032e-03,  -2.5284969287895365e-03,   2.0205225499012158e-02,  -3.3663287335002678e-02,   2.4609693899947160e-02,  -6.8108938485238468e-03 },
	{ 4.6110085440020632e-01,   3.7037908144910148e-06,   6.0138650621226475e-02,   8.2734355981983754e-04,  -3.4452522463652713e-03,  -6.5863408372024423e-03,   2.5523234984575538e-02,  -3.7716510669270065e-02,   2.6192411091869872e-02,  -7.0481435282090388e-03 },
	{ 4.6507375356073921e-01,   3.9524247892330777e-06,   5.3791560245343817e-02,   9.0903684264276308e-04,  -3.4421425607433775e-03,  -4.8560125733274617e-03,   2.2129510451122769e-02,  -3.4029924228889286e-02,   2.4047639981290558e-02,  -6.5342707175659598e-03 },
	{ 4.6860976034632418e-01,   3.2249864734268385e-06,   4.8204609991927327e-02,   7.2309841186779522e-04,  -2.1408826423794380e-03,  -7.2955193090820103e-03,   2.6006810019680415e-02,  -3.7701050612668041e-02,   2.5920977407622559e-02,  -6.9344606379218021e-03 },
	{ 4.7175861229370536e-01,   2.8048572175976005e-06,   4.3245131406322912e-02,   7.6159856308777307e-04,  -2.3823094688850688e-03,  -4.8122726784640690e-03,   2.0579741616529645e-02,  -3.1397320455653244e-02,   2.2136876519653015e-02,  -6.0135830062790774e-03 },
	{ 4.7456459335160162e-01,   2.8417696231741729e-06,   3.8843988370160787e-02,   7.7926670735450898e-04,  -2.2731421731805312e-03,  -4.2516092216828838e-03,   1.9379007610041299e-02,  -3.0013869720278308e-02,   2.1296342856658157e-02,  -5.8053825280239835e-03 },
	{ 4.7706700658919132e-01,   2.7752737477015899e-06,   3.4939477451274570e-02,   7.6566523301835332e-04,  -2.0393289869389264e-03,  -4.2499675619183108e-03,   1.9219391015212750e-02,  -2.9745351112069329e-02,   2.1099286486787605e-02,  -5.7514328855177155e-03 },
	{ 4.7930063189057454e-01,   2.7549469443499675e-06,   3.1467232821043467e-02,   7.6598212820044864e-04,  -1.9200056635781948e-03,  -4.0250104493679828e-03,   1.8661052999959793e-02,  -2.9050648423435632e-02,   2.0656473570852540e-02,  -5.6382337106697378e-03 },
	{ 4.8129620119679456e-01,   2.5144073356742069e-06,   2.8381833920789745e-02,   7.0858879223578697e-04,  -1.5370190358225955e-03,  -4.6468046903100912e-03,   1.9520223899235134e-02,  -2.9726655069680419e-02,   2.0926506535033695e-02,  -5.6791505148794386e-03 },
	{ 4.8308081543818293e-01,   4.9270950963631321e-06,   2.5569640810857663e-02,   1.1997948886346421e-03,  -3.6347477171148057e-03,   1.0211746839559055e-03,   1.0772807487228420e-02,  -2.1749405606897199e-02,   1.6956387829850428e-02,  -4.8482511413112661e-03 },
	{ 4.8467842919354887e-01,   4.5859380168877806e-06,   2.3118285142842865e-02,   1.1115789512814445e-03,  -3.1153407894635166e-03,  -2.0768865579157136e-04,   1.2822442133256118e-02,  -2.3767568261973793e-02,   1.8035196240816731e-02,  -5.0900434289360419e-03 },
	{ 4.8611010883805972e-01,   4.6216910085661311e-06,   2.0919053199506266e-02,   1.1034297294827411e-03,  -2.9798532800668909e-03,  -4.5528213013312779e-04,   1.3289935897773830e-02,  -2.4259850981252384e-02,   1.8304458944840007e-02,  -5.1499611422514135e-03 },
	{ 4.8739445333760972e-01,   4.0681659907448875e-06,   1.8963335087008204e-02,   9.7421275057740786e-04,  -2.1971452047182538e-03,  -2.8298405368332169e-03,   1.7800989204260986e-02,  -2.9175307528930716e-02,   2.1141632292710710e-02,  -5.8216387010361359e-03 },
	{ 4.8854784306756249e-01,   4.6417616585969768e-06,   1.7183820455798582e-02,   1.1365541869849949e-03,  -3.0202538182493299e-03,  -2.2839752637082711e-04,   1.3213225540312123e-02,  -2.4463424833811587e-02,   1.8532392211454862e-02,  -5.2199500785263808e-03 },
	{ 4.8958475238667981e-01,   4.6140084908330437e-06,   1.5601362428355969e-02,   1.1322211803985738e-03,  -2.9811834187967179e-03,  -1.8707497565628728e-04,   1.3062932303000707e-02,  -2.4245279520982876e-02,   1.8382185740847490e-02,  -5.1797001322029246e-03 },
	{ 4.9051793867102589e-01,   5.1101924896101991e-06,   1.4165388866388184e-02,   1.2809368830630774e-03,  -3.7933222911306075e-03,   2.5022063910000725e-03,   8.0151748861680971e-03,  -1.8749955062958179e-02,   1.5176250204604003e-02,  -4.4058930945993779e-03 },
	{ 4.9135868233799335e-01,   5.0937693574226728e-06,   1.2886079313375376e-02,   1.2790767485171273e-03,  -3.7754276845589629e-03,   2.5614821606723126e-03,   7.8483470497303642e-03,  -1.8526747771829832e-02,   1.5028808204078814e-02,  -4.3673416266756249e-03 },
	{ 4.9211694443551995e-01,   5.0790512649001585e-06,   1.1733781826009704e-02,   1.2774141535487615e-03,  -3.7612739074575074e-03,   2.6149449686272419e-03,   7.6981182719464414e-03,  -1.8325728229683591e-02,   1.4896044277520559e-02,  -4.3326312122644595e-03 },
	{ 4.9280153927251469e-01,   5.0658383985435762e-06,   1.0694655292724065e-02,   1.2759251552552087e-03,  -3.7501021693060466e-03,   2.6632136778061977e-03,   7.5626707784977043e-03,  -1.8144476614907035e-02,   1.4776353688830568e-02,  -4.3013412209802482e-03 },
	{ 4.9342027064485139e-01,   5.0539580698227837e-06,   9.7564958492739606e-03,   1.2745892173597895e-03,  -3.7413099303194031e-03,   2.7068381459685043e-03,   7.4404025253897998e-03,  -1.7980857543079765e-02,   1.4668321236058546e-02,  -4.2731009148155863e-03 },
	{ 4.9398005403224532e-01,   5.0432592715310420e-06,   8.9085288632304582e-03,   1.2733884938143092e-03,  -3.7344172078519478e-03,   2.7463053147585015e-03,   7.3299020914419089e-03,  -1.7832986768553383e-02,   1.4570698630450352e-02,  -4.2475833495245752e-03 },
	{ 4.9448702355353991e-01,   5.0336107149107079e-06,   8.1412289566742402e-03,   1.2723075629423874e-03,  -3.7290414843482722e-03,   2.7820480490845512e-03,   7.2299221737921471e-03,  -1.7699198078844347e-02,   1.4482382531241456e-02,  -4.2244996668614476e-03 },
	{ 4.9494662557060048e-01,   5.0248969198918303e-06,   7.4461641831788938e-03,   1.2713329274447460e-03,  -3.7248769490361155e-03,   2.8144501493443386e-03,   7.1393606340279803e-03,  -1.7578017470441409e-02,   1.4402397190679039e-02,  -4.2035944475173892e-03 },
	{ 4.9536370061612434e-01,   5.0170166865370902e-06,   6.8158608858510661e-03,   1.2704527890150530e-03,  -3.7216789291960595e-03,   2.8438525032470352e-03,   7.0572419863310643e-03,  -1.7468139580159914e-02,   1.4329878627904691e-02,  -4.1846415513191459e-03 },
	{ 4.9574255512218041e-01,   5.0098808017651209e-06,   6.2436863815094057e-03,   1.2696568467163161e-03,  -3.7192516688264732e-03,   2.8705590939352987e-03,   6.9826998624193948e-03,  -1.7368404794979142e-02,   1.4264059751440072e-02,  -4.1674402846183511e-03 },
	{ 4.9608702424598627e-01,   5.0034113074914899e-06,   5.7237469707871469e-03,   1.2689361132061094e-03,  -3.7174384597165044e-03,   2.8948412409590674e-03,   6.9149638075032271e-03,  -1.7277782118981122e-02,   1.4204258815880166e-02,  -4.1518123034620658e-03 },
	{ 4.9640052692803005e-01,   4.9975386913203579e-06,   5.2507991886159289e-03,   1.2682825761771710e-03,  -3.7161130767344730e-03,   2.9169392364565283e-03,   6.8533515895978780e-03,  -1.7195357349919504e-02,   1.4149871168228856e-02,  -4.1375994269401417e-03 },
	{ 4.9668611417357966e-01,   4.9922011760017426e-06,   4.8201724258234435e-03,   1.2676891882961172e-03,  -3.7151743777030788e-03,   2.9370683469096548e-03,   6.7972537799505517e-03,  -1.7120314258136204e-02,   1.4100357246206841e-02,  -4.1246605132982950e-03 },
	{ 4.9694651142200591e-01,   4.9873452168114341e-06,   4.4277013920179797e-03,   1.2671498207623699e-03,  -3.7145414503356733e-03,   2.9554215416283114e-03,   6.7461264607118210e-03,  -1.7051924703991972e-02,   1.4055235908017494e-02,  -4.1128697662315972e-03 },
	{ 4.9718415575683911e-01,   4.9829217880770216e-06,   4.0696671523789973e-03,   1.2666589031482545e-03,  -3.7141487550798047e-03,   2.9721698920184281e-03,   6.6994861717830645e-03,  -1.6989540654321900e-02,   1.4014078776199312e-02,  -4.1021151341738005e-03 },
	{ 4.9740122861160618e-01,   4.9788891152147130e-06,   3.7427454517739989e-03,   1.2662117029549336e-03,  -3.7139444921194809e-03,   2.9874687143092160e-03,   6.6568975580594270e-03,  -1.6932579956119298e-02,   1.3976501563774946e-02,  -4.0922961870819563e-03 }
};

double sigmaT_fiberGaussian(double stddev, double sinTheta) {
	/* Undo the nonlinear mapping */
	const double factor = SIGMA_T_ELEMENTS / std::pow(SIGMA_T_RANGE_MAX, 0.25);
	double pos = std::pow(stddev, 0.25) * factor - 1;

	/* Clamp to the supported parameter range */
	pos = std::min(std::max(0.0, pos), (double) (SIGMA_T_ELEMENTS-1));

	/* Linearly interpolate coefficients using \c alpha */
	int idx0 = (int) std::floor(pos), idx1 = (int) std::ceil(pos);
	double alpha = pos - idx0, base = 1, result = 0;

	SAssert(
		idx0 >= 0 && idx0 < SIGMA_T_ELEMENTS &&
		idx1 >= 0 && idx1 < SIGMA_T_ELEMENTS
	);

	/* Compute the expansion */
	for (int i=0; i<SIGMA_T_COEFFS; ++i) {
		result += base * ((1-alpha) * fiberGaussianCoeffs[idx0][i]
				+ alpha * fiberGaussianCoeffs[idx1][i]);
		base *= sinTheta;
	}

	return result;
}

MTS_NAMESPACE_END

#endif /* __MICROFLAKE_SIGMA_T_H */
