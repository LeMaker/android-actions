void yuv420_to_rgb565(const unsigned char *src, unsigned short *dst, int width, int height)
{
  int line, col, linewidth;
  int y, u, v, yy, vr, ug, vg, ub;
  int r, g, b;
  const unsigned char *py, *pu, *pv;

  linewidth = width >> 1;
  py = src;
  pu = py + (width * height);
  pv = pu + (width * height) / 4;

  y = *py++;
  yy = y << 8;
  u = *pu - 128;
  ug = 88 * u;
  ub = 454 * u;
  v = *pv - 128;
  vg = 183 * v;
  vr = 359 * v;

  for (line = 0; line < height; line++) {
    for (col = 0; col < width; col++) {
      r = (yy + vr) >> 8;
      g = (yy - ug - vg) >> 8;
      b = (yy + ub ) >> 8;

      if (r < 0) r = 0;
      if (r > 255) r = 255;
      if (g < 0) g = 0;
      if (g > 255) g = 255;
      if (b < 0) b = 0;
      if (b > 255) b = 255;
      *dst++ = (((unsigned short)r>>3)<<11) | (((unsigned short)g>>2)<<5) | (((unsigned short)b>>3)<<0);
  
      y = *py++;
      yy = y << 8;
      if (col & 1) {
    pu++;
    pv++;

    u = *pu - 128;
    ug = 88 * u;
    ub = 454 * u;
    v = *pv - 128;
    vg = 183 * v;
    vr = 359 * v;
      }
    }
    if ((line & 1) == 0) {
      pu -= linewidth;
      pv -= linewidth;
    }
  }
}
