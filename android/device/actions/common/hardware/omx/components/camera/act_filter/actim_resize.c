#include "actim_resize.h"
#ifndef _OPENMAX_V1_2_
#include "ACT_OMX_Common_V1_2__V1_1.h"
#endif
#include "perf.h"
#include "log.h"

#define RESIZE_PERF_TEST 0

static int subsample_image(unsigned char *pIn, int nInW, int nInH, int stride_w, 
                            unsigned char *pUV, unsigned char *pOut, int nOW, int nOH)
{
    unsigned char *psrc_y = pIn;
    unsigned short *psrc_uv = (unsigned short *)(pUV);
    unsigned char *pdst_y = pOut;
    unsigned short *pdst_uv = (unsigned short *)(pOut + nOW * nOH);
    int nXstep = nInW * 1024 / nOW;
    int nYstep = nInH * 1024 / nOH;
    unsigned int nX = 0;
    unsigned int nY = 0;
    unsigned int x, y, i, j;
    int W2 = nOW / 2;
    int H2 = nOH / 2;

    OMXDBUG(OMXDBUG_ERR, "nOH nOW %d,%d,%d,%d,%d,%d,%d,%d \n", nInW, nInH, nOW, nOH, nXstep, nYstep, W2, H2);



    for(i = 0; i < nOH; i++)
    {
        unsigned char *pTemp;
        nX = 0;
        y = nY >> 10;
        pTemp = psrc_y + y * stride_w;

        for(j = 0; j < nOW; j++)
        {
            x = nX >> 10;
            *pdst_y++ = *(pTemp + x);
            nX += nXstep;
        }

        nY += nYstep;
    }

    nY = 0;

    for(i = 0; i < H2; i++)
    {
        unsigned short *pTemp;
        nX = 0;
        y = nY >> 10;
        pTemp = (psrc_uv + y * stride_w / 2);

        for(j = 0; j < W2; j++)
        {
            unsigned short tempVal = 0;
            x = nX >> 10;
            tempVal = *(pTemp + x);
            *pdst_uv++ = tempVal;


            nX += nXstep;
        }

        nY += nYstep;
    }

    return 0;
}

static int subsample_image_u_v(unsigned char *pIn, int nInW, int nInH, int stride_w, unsigned char *pU, unsigned char *pV, unsigned char *pOut, int nOW, int nOH)
{
    unsigned char *psrc_y = pIn;
    unsigned char *psrc_u = (unsigned char *)(pU);
    unsigned char *psrc_v = (unsigned char *)(pV);
    unsigned char *pdst_y = pOut;
    unsigned char *pdst_u = (unsigned char *)(pOut + nOW * nOH);
    unsigned char *pdst_v = (unsigned char *)(pdst_u + nOW / 2 * nOH / 2);
    int nXstep = nInW * 1024 / nOW;
    int nYstep = nInH * 1024 / nOH;
    unsigned int nX = 0;
    unsigned int nY = 0;
    unsigned int x, y, i, j;
    int W2 = nOW / 2;
    int H2 = nOH / 2;
    int Wl = ((W2 + 15) / 16) * 16 - W2;

    //OMXDBUG(OMXDBUG_ERR,"nOH nOW %d,%d,%d,%d,%d,%d,%d,%d \n",nInW,nInH,nOW,nOH,nXstep,nYstep,W2,H2);

    int dws = ((nOW + 15) / 16) * 16 - nOW;

    pdst_u = (unsigned char *)(pOut + (nOW + dws) * nOH);
    pdst_v = (unsigned char *)(pdst_u + (W2 + Wl) * nOH / 2);

    for(i = 0; i < nOH; i++)
    {
        unsigned char *pTemp;
        nX = 0;
        y = nY >> 10;
        pTemp = psrc_y + y * stride_w;

        for(j = 0; j < nOW; j++)
        {
            x = nX >> 10;
            *pdst_y++ = *(pTemp + x);
            nX += nXstep;
        }

        pdst_y += dws;
        nY += nYstep;
    }

    nY = 0;

    for(i = 0; i < H2; i++)
    {
        unsigned char *pTemp;
        nX = 0;
        y = nY >> 10;
        pTemp = (psrc_u + y * stride_w / 2);

        for(j = 0; j < W2; j++)
        {
            unsigned char tempVal = 0;
            x = nX >> 10;
            tempVal = *(pTemp + x);
            *pdst_u++ = tempVal;


            nX += nXstep;
        }

        pdst_u += Wl;
        nY += nYstep;
    }

    nY = 0;

    for(i = 0; i < H2; i++)
    {
        unsigned char *pTemp;
        nX = 0;
        y = nY >> 10;
        pTemp = (psrc_v + y * stride_w / 2);

        for(j = 0; j < W2; j++)
        {
            unsigned char tempVal = 0;
            x = nX >> 10;
            tempVal = *(pTemp + x);
            *pdst_v++ = tempVal;


            nX += nXstep;
        }

        pdst_v += Wl;
        nY += nYstep;
    }

    return 0;
}


#define FILTER_W 2

static void downscale_img(unsigned char *img, int w, int h, int stride_w, int stride_h, unsigned char *outimg, int dw, int dh)
{
    float xstep = (float)w / (float)dw;
    float ystep = (float)h / (float)dh;
    unsigned char *psrc = img;
    unsigned char *pdst = outimg;
    int i, j;
    int x, y;
    int i1, j1;

    for(i = 0; i < dh; i++)
    {
        y = i * ystep;

        for(j = 0; j < dw; j++)
        {
            x = j * xstep;

            if((i == 0) || (i == dh - 1) || (j == 0) || (j == dw - 1))
            {
                int i_s = -FILTER_W;
                int i_e =  FILTER_W;
                int j_s = -FILTER_W;
                int j_e =  FILTER_W;
                int sumP = 0;
                int sumCoef = 0;
                unsigned char *pTmep = psrc + y * stride_w + x;

                if(x - FILTER_W < 0) { j_s = 0; }

                if(x + FILTER_W > w - 1) { j_e = 0; }

                if(y - FILTER_W < 0) { i_s = 0; }

                if(y + FILTER_W > h - 1) { i_e = 0; }

                for(i1 = i_s; i1 < i_e; i1++)
                {
                    for(j1 = j_s; j1 < j_e; j1++)
                    {
                        sumP = sumP + pTmep[i1 * stride_w + j1]; //*filter7x7[i1+3][j1+3];
                        sumCoef = sumCoef + 1; // + filter7x7[i1+3][j1+3];
                    }
                }

                pdst[i * dw + j] = sumP / sumCoef;

                if(pdst[i * dw + j] > 255)
                {
                    pdst[i * dw + j] = 255;
                }

            }
            else
            {

                int sumP = 0;
                unsigned char *pTmep = psrc + (y - FILTER_W) * stride_w + x - FILTER_W;

                for(i1 = 0; i1 < FILTER_W * 2 + 1; i1++)
                {
                    for(j1 = 0; j1 < FILTER_W * 2 + 1; j1++)
                    {
                        sumP = sumP + pTmep[i1 * stride_w + j1]; //*filter7x7[i1][j1];
                    }
                }

                pdst[i * dw + j] = sumP / ((FILTER_W * 2 + 1) * (FILTER_W * 2 + 1));

                if(pdst[i * dw + j] > 255)
                {
                    pdst[i * dw + j] = 255;
                }

            }
        }
    }
}

static void downscale_img_uv(unsigned char *img, int w, int h, int stride_w, int stride_h, unsigned char *outimg, int dw, int dh)
{
    float xstep = (float)w / (float)dw;
    float ystep = (float)h / (float)dh;
    unsigned char *psrc = img;
    unsigned char *pdst = outimg;
    int src_stride_w = stride_w * 2;
    int dst_stride_w = dw * 2;
    int i, j;
    int x, y, k;
    int i1, j1;
    int k_s = (2 * FILTER_W + 1) * (2 * FILTER_W + 1);

    for(i = 0; i < dh; i++)
    {
        y = i * ystep;

        for(j = 0; j < dw; j++)
        {
            x = j * xstep;

            if((i == 0) || (i == dh - 1) || (j == 0) || (j == dw - 1))
            {
                int i_s = -FILTER_W;
                int i_e =  FILTER_W;
                int j_s = -FILTER_W;
                int j_e =  FILTER_W;
                int sumP = 0;
                int sumCoef = 0;
                unsigned char *pTmep = psrc + y * src_stride_w + x * 2;

                if(x - FILTER_W < 0) { j_s = 0; }

                if(x + FILTER_W > w - 1) { j_e = 0; }

                if(y - FILTER_W < 0) { i_s = 0; }

                if(y + FILTER_W > h - 1) { i_e = 0; }

                for(i1 = i_s; i1 < i_e; i1++)
                {
                    for(j1 = j_s; j1 < j_e; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2]; //*filter7x7[i1+3][j1+3];
                        sumCoef = sumCoef + 1; //+ filter7x7[i1+3][j1+3];
                    }
                }

                pdst[i * dst_stride_w + j * 2] = sumP / sumCoef;

                if(pdst[i * dst_stride_w + j * 2] > 255)
                {
                    pdst[i * dst_stride_w + j * 2] = 255;
                }

                sumP = 0;
                sumCoef = 0;

                for(i1 = i_s; i1 < i_e; i1++)
                {
                    for(j1 = j_s; j1 < j_e; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2 + 1]; //*filter7x7[i1+3][j1+3];
                        sumCoef = sumCoef + 1; //+ filter7x7[i1+3][j1+3];
                    }
                }

                pdst[i * dst_stride_w + j * 2 + 1] = sumP / sumCoef;

                if(pdst[i * dst_stride_w + j * 2 + 1] > 255)
                {
                    pdst[i * dst_stride_w + j * 2 + 1] = 255;
                }

            }
            else
            {

                int sumP = 0;
                unsigned char *pTmep = psrc + (y - FILTER_W) * src_stride_w + (x - FILTER_W) * 2;

                for(i1 = 0; i1 < 2 * FILTER_W + 1; i1++)
                {
                    for(j1 = 0; j1 < 2 * FILTER_W + 1; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2]; //*filter7x7[i1][j1];
                    }
                }

                pdst[i * dst_stride_w + j * 2] = sumP / k_s;

                if(pdst[i * dst_stride_w + j * 2] > 255)
                {
                    pdst[i * dst_stride_w + j * 2] = 255;
                }

                sumP = 0;

                for(i1 = 0; i1 < 2 * FILTER_W + 1; i1++)
                {
                    for(j1 = 0; j1 < 2 * FILTER_W + 1; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2 + 1]; //*filter7x7[i1][j1];
                    }
                }

                pdst[i * dst_stride_w + j * 2 + 1] = sumP / k_s;

                if(pdst[i * dst_stride_w + j * 2 + 1] > 255)
                {
                    pdst[i * dst_stride_w + j * 2 + 1] = 255;
                }
            }
        }
    }
}

static void downscale_img_u_v(unsigned char *img, int w, int h, int stride_w, int stride_h, unsigned char *outimg, int dw, int dh)
{
    float xstep = (float)w / (float)dw;
    float ystep = (float)h / (float)dh;
    unsigned char *psrc = img;
    unsigned char *pdst = outimg;
    int src_stride_w = stride_w;
    int dst_stride_w = ((dw + 15) / 16) * 16;
    int i, j;
    int x, y, k;
    int i1, j1;
    int k_s = (2 * FILTER_W + 1) * (2 * FILTER_W + 1);

    for(i = 0; i < dh; i++)
    {
        y = i * ystep;

        for(j = 0; j < dw; j++)
        {
            x = j * xstep;

            if((i == 0) || (i == dh - 1) || (j == 0) || (j == dw - 1))
            {
                int i_s = -FILTER_W;
                int i_e =  FILTER_W;
                int j_s = -FILTER_W;
                int j_e =  FILTER_W;
                int sumP = 0;
                int sumCoef = 0;
                unsigned char *pTmep = psrc + y * src_stride_w + x * 2;

                if(x - FILTER_W < 0) { j_s = 0; }

                if(x + FILTER_W > w - 1) { j_e = 0; }

                if(y - FILTER_W < 0) { i_s = 0; }

                if(y + FILTER_W > h - 1) { i_e = 0; }

                for(i1 = i_s; i1 < i_e; i1++)
                {
                    for(j1 = j_s; j1 < j_e; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2]; //*filter7x7[i1+3][j1+3];
                        sumCoef = sumCoef + 1; //+ filter7x7[i1+3][j1+3];
                    }
                }

                pdst[i * dst_stride_w + j * 2] = sumP / sumCoef;

                if(pdst[i * dst_stride_w + j * 2] > 255)
                {
                    pdst[i * dst_stride_w + j * 2] = 255;
                }

                sumP = 0;
                sumCoef = 0;

                for(i1 = i_s; i1 < i_e; i1++)
                {
                    for(j1 = j_s; j1 < j_e; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2 + 1]; //*filter7x7[i1+3][j1+3];
                        sumCoef = sumCoef + 1; //+ filter7x7[i1+3][j1+3];
                    }
                }

                pdst[i * dst_stride_w + j * 2 + 1] = sumP / sumCoef;

                if(pdst[i * dst_stride_w + j * 2 + 1] > 255)
                {
                    pdst[i * dst_stride_w + j * 2 + 1] = 255;
                }

            }
            else
            {

                int sumP = 0;
                unsigned char *pTmep = psrc + (y - FILTER_W) * src_stride_w + (x - FILTER_W) * 2;

                for(i1 = 0; i1 < 2 * FILTER_W + 1; i1++)
                {
                    for(j1 = 0; j1 < 2 * FILTER_W + 1; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2]; //*filter7x7[i1][j1];
                    }
                }

                pdst[i * dst_stride_w + j * 2] = sumP / k_s;

                if(pdst[i * dst_stride_w + j * 2] > 255)
                {
                    pdst[i * dst_stride_w + j * 2] = 255;
                }

                sumP = 0;

                for(i1 = 0; i1 < 2 * FILTER_W + 1; i1++)
                {
                    for(j1 = 0; j1 < 2 * FILTER_W + 1; j1++)
                    {
                        sumP = sumP + pTmep[i1 * src_stride_w + j1 * 2 + 1]; //*filter7x7[i1][j1];
                    }
                }

                pdst[i * dst_stride_w + j * 2 + 1] = sumP / k_s;

                if(pdst[i * dst_stride_w + j * 2 + 1] > 255)
                {
                    pdst[i * dst_stride_w + j * 2 + 1] = 255;
                }
            }
        }
    }
}
/* 双线性缩放函数 */
static unsigned char BilInear3(unsigned char *PColor0, unsigned char *PColor1, unsigned int u_8, unsigned int v_8)
{
    unsigned int pm3_8 = ((u_8 * v_8) >> 6);
    unsigned int pm2_8 = (u_8 - pm3_8); //((u_8-pm3_8)==1)?0:
    unsigned int pm1_8 = (v_8 - pm3_8); //((v_8-pm3_8)==1)?0:
    unsigned int pm0_8 = (64 - pm1_8 - pm2_8 - pm3_8); //((64-pm1_8-pm2_8-pm3_8)==1)?0:
    unsigned int Color = *PColor0;
    unsigned int Y0 = Color * pm0_8;

    Color = PColor0[1];
    Y0 += Color * pm2_8;
    Color = *PColor1;
    Y0 += Color * pm1_8;
    Color = PColor1[1];
    Y0 += Color * pm3_8;

    return (unsigned char)(Y0 >> 6);
}


static unsigned char BilInear_uv(unsigned char *PColor0, unsigned char *PColor1, unsigned int u_8, unsigned int v_8)
{
    unsigned int pm3_8 = ((u_8 * v_8) >> 6);
    unsigned int pm2_8 = (u_8 - pm3_8); //((u_8-pm3_8)==1)?0:
    unsigned int pm1_8 = (v_8 - pm3_8); //((v_8-pm3_8)==1)?0:
    unsigned int pm0_8 = (64 - pm1_8 - pm2_8 - pm3_8); //((64-pm1_8-pm2_8-pm3_8)==1)?0:
    unsigned int Color = *PColor0;
    unsigned int Y0 = Color * pm0_8;

    Color = PColor0[2];
    Y0 += Color * pm2_8;
    Color = *PColor1;
    Y0 += Color * pm1_8;
    Color = PColor1[2];
    Y0 += Color * pm3_8;

    return (unsigned char)(Y0 >> 6);
}


/* 亮度进行缩放 */
static void PicZoom_BilInear4(unsigned char *Src, unsigned int SrcWidth, unsigned int SrcHeight, int stride_w, unsigned char *pUV, \
                       unsigned char *Dst, unsigned int DstWidth, unsigned int DstHeight)
{
    unsigned int x, y;
    unsigned int xrIntFloat_16 = ((SrcWidth - 1) << 16) / DstWidth ;
    unsigned int yrIntFloat_16 = ((SrcHeight - 1) << 16) / DstHeight ;

    unsigned int dst_width = DstWidth;
    int Src_byte_width = stride_w;
    unsigned char *pDstLine = Dst;
    unsigned char *pSrc = Src;
    unsigned int srcy_16 = 0;
    int idy = 0, idx = 0;
    unsigned char *psrcUV = pUV;
    unsigned char *pdstUV = Dst + DstWidth * DstHeight;

    for(y = 0; y < DstHeight; ++y)
    {
        unsigned int v_8 = (srcy_16 & 0xFFFF) >> 10;
        unsigned char *PSrcLineColor = pSrc + Src_byte_width * (srcy_16 >> 16);
        unsigned int srcx_16 = 0;

        for(x = 0; x < DstWidth; ++x)
        {
            unsigned char *PColor0 = &PSrcLineColor[srcx_16 >> 16];
            pDstLine[x] = BilInear3(PColor0, PColor0 + Src_byte_width, (srcx_16 & 0xFFFF) >> 10, v_8);
            srcx_16 += xrIntFloat_16;
        }

        srcy_16 += yrIntFloat_16;
        pDstLine += DstWidth;
    }

    srcy_16 = 0;

    for(y = 0; y < DstHeight / 2; ++y)
    {
        unsigned int v_8 = (srcy_16 & 0xFFFF) >> 10;
        unsigned char *PSrcLineColor = psrcUV + Src_byte_width * (srcy_16 >> 16);
        unsigned int srcx_16 = 0;

        for(x = 0; x < DstWidth / 2; ++x)
        {
            int x1 = srcx_16 >> 16;
            int u_8 = (srcx_16 & 0xFFFF) >> 10;
            unsigned char *PColor0 = &PSrcLineColor[x1 * 2];
            pdstUV[2 * x] = BilInear_uv(PColor0, PColor0 + Src_byte_width, u_8, v_8);
            PColor0 = &PSrcLineColor[x1 * 2 + 1];
            pdstUV[2 * x + 1] = BilInear_uv(PColor0, PColor0 + Src_byte_width, u_8, v_8);
            srcx_16 += xrIntFloat_16;
        }

        srcy_16 += yrIntFloat_16;
        pdstUV += DstWidth;
    }
}


static void PicZoom_BilInear_u_v(unsigned char *Src, unsigned int SrcWidth, unsigned int SrcHeight, int stride_w, unsigned char *pU, unsigned char *pV, \
                          unsigned char *Dst, unsigned int DstWidth, unsigned int DstHeight)
{
    unsigned int x, y;
    unsigned int xrIntFloat_16 = ((SrcWidth - 1) << 16) / DstWidth ;
    unsigned int yrIntFloat_16 = ((SrcHeight - 1) << 16) / DstHeight ;

    unsigned int dst_width = DstWidth;
    int Src_byte_width = stride_w;
    unsigned char *pDstLine = Dst;
    unsigned char *pSrc = Src;
    unsigned int srcy_16 = 0;
    int idy = 0, idx = 0;
    unsigned char *psrcU = pU;
    unsigned char *psrcV = pV;
    unsigned char *pdstU = Dst + DstWidth * DstHeight;
    unsigned char *pdstV = pdstU + DstWidth / 2 * DstHeight / 2;
    int DstWidth2 = DstWidth / 2;
    int DstWidthAlign = (((DstWidth + 15) / 16) * 16);
    DstWidth2 = ((DstWidth2 + 15) / 16) * 16;

    pdstU = Dst + DstWidthAlign * DstHeight;
    pdstV = pdstU + DstWidth2 * DstHeight / 2;

    for(y = 0; y < DstHeight; ++y)
    {
        unsigned int v_8 = (srcy_16 & 0xFFFF) >> 10;
        unsigned char *PSrcLineColor = pSrc + Src_byte_width * (srcy_16 >> 16);
        unsigned int srcx_16 = 0;

        for(x = 0; x < DstWidth; ++x)
        {
            unsigned char *PColor0 = &PSrcLineColor[srcx_16 >> 16];
            pDstLine[x] = BilInear3(PColor0, PColor0 + Src_byte_width, (srcx_16 & 0xFFFF) >> 10, v_8);
            srcx_16 += xrIntFloat_16;
        }

        srcy_16 += yrIntFloat_16;
        pDstLine += DstWidthAlign;
    }

    srcy_16 = 0;

    for(y = 0; y < DstHeight / 2; ++y)
    {
        unsigned int v_8 = (srcy_16 & 0xFFFF) >> 10;
        unsigned char *PSrcLineColor = psrcU + (Src_byte_width / 2) * (srcy_16 >> 16);
        unsigned char *PSrcLineColor_v = psrcV + (Src_byte_width / 2) * (srcy_16 >> 16);
        unsigned int srcx_16 = 0;

        for(x = 0; x < DstWidth / 2; ++x)
        {
            int x1 = srcx_16 >> 16;
            int u_8 = (srcx_16 & 0xFFFF) >> 10;
            unsigned char *PColor0 = &PSrcLineColor[x1];
            unsigned char *PColor0_v = &PSrcLineColor_v[x1];
            pdstU[x] = BilInear3(PColor0, PColor0 + Src_byte_width / 2, u_8, v_8);
            pdstV[x] = BilInear3(PColor0_v, PColor0_v + Src_byte_width / 2, u_8, v_8);
            srcx_16 += xrIntFloat_16;
        }

        srcy_16 += yrIntFloat_16;
        pdstU += DstWidth2;
        pdstV += DstWidth2;
    }
}

int downsample_image(unsigned char *pIn, int nInW, int nInH, unsigned char *pOut, int nOW, int nOH, int mode)
{
    int xRatio = nInW * 1024 / nOW;
    int yRatio = nInH * 1024 / nOH;
    int xyRatio = xRatio;
    unsigned char *pSrc = pIn;
    unsigned char *pSUV = pIn;
    unsigned char *pSV = pIn;
    int stride = nInW;
    int nW = nInW;
    int nH = nInH;
    int x, y;
    //FILE *fy = fopen("/data/out.yuv","wb");

#if RESIZE_PERF_TEST
    double t0 = now_ms();
#endif

    if(xRatio > yRatio) { xyRatio = yRatio; }

    nW = xyRatio * nOW / 1024;
    nH = xyRatio * nOH / 1024;
    nW = ((nW + 15) / 16) * 16;
    nH = ((nH + 3) / 4) * 4;

//	    y = (((nInH - nH) / 2) & (~0x3));
//	    x = ((nInW - nW) / 2) & (~0xf);
    y = ((nInH - nH) / 2) & (~0x1);
    x = ((nInW - nW) / 2) & (~0x1);



    if(mode == OMX_COLOR_FormatYUV420Planar || mode == OMX_COLOR_FormatYVU420Planar)
    {
        int strideuv = nOW / 2;
        strideuv = ((strideuv + 15) / 16) * 16;

        OMXDBUG(OMXDBUG_VERB, "Y420 %d,%d,nW %d,nH %d %d\n", x, y, nW, nH, strideuv);
        pSrc = pIn +  y * stride + x ;
        pSUV = pIn +  nInW * nInH + y / 2 * stride / 2 + x / 2 ;
        pSV = pSUV + nInW / 2 * nInH / 2;

        if(nInW > 4 * nOW)
        {
            downscale_img(pSrc, nW, nH, nInW, nInH, pOut, nOW, nOH);
            downscale_img_u_v(pSUV, nW / 2, nH / 2, nInW / 2, nInH / 2, pOut + nOW * nOH, nOW / 2, nOH / 2);
            downscale_img_u_v(pSV, nW / 2, nH / 2, nInW / 2, nInH / 2, pOut + nOW * nOH + strideuv * nOH / 2 , nOW / 2, nOH / 2);
        }
        else if(nInW >= 2 * nOW)
        {
            PicZoom_BilInear_u_v(pSrc, nW, nH, nInW, pSUV, pSV, pOut, nOW, nOH);
        }
        else
        {
            subsample_image_u_v(pSrc, nW, nH, nInW, pSUV, pSV, pOut, nOW, nOH);
        }

        //fwrite(pOut,1,nOH*nOW*3/2,fy);
        //fclose(fy);
    }
    else if(mode == OMX_COLOR_FormatYUV420SemiPlanar || mode == OMX_COLOR_FormatYVU420SemiPlanar)
    {

        OMXDBUG(OMXDBUG_VERB, "Y420S %d,%d,nW %d,nH %d \n", x, y, nW, nH);
        pSrc = pIn +  y * stride + x ;
        pSUV = pIn +  nInW * nInH + y / 2 * stride + x ;

        if(nInW > 4 * nOW)
        {
            downscale_img(pSrc, nW, nH, nInW, nInH, pOut, nOW, nOH);
            downscale_img_uv(pSUV, nW / 2, nH / 2, nInW / 2, nInH / 2, pOut + nOW * nOH, nOW / 2, nOH / 2);
        }
        else if(nInW >= 2 * nOW)
        {
            PicZoom_BilInear4(pSrc, nW, nH, nInW, pSUV, pOut, nOW, nOH);
        }
        else
        {
            subsample_image(pSrc, nW, nH, nInW, pSUV, pOut, nOW, nOH);
        }

    }
    else
    {
        OMXDBUG(OMXDBUG_ERR, "not support this format");
    }



    //bicubicresize_y(pIn,nInW,nInH,pOut,nOW,nOH);
    //bicubicresize_uv(pIn+nInW*nInH,nInW,nInH,pOut+nOW*nOH,nOW,nOH);

#if RESIZE_PERF_TEST
    OMXDBUG(OMXDBUG_PARAM, "%s from %dx%d to %dx%d: %gms\n", __func__, nInW, nInH, nOW, nOH, now_ms() - t0);
#endif

    return 0;
}


