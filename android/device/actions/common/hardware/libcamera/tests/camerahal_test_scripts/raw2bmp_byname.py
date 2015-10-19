
import re
import sys,os

from raw2bmp import raw_to_bmp


def main():
    if len(sys.argv) < 2:
        print "%s parameter error" % sys.argv[0]
        sys.exit();

    m = re.match( r"\d*_(img|preview)\d*_(\d*)_(\d*)_(\w*)\.(raw|yuv)", sys.argv[1]);

    if m is None:
        sys.exit();

    width = int(m.group(2))
    height = int(m.group(3))
    if m.group(4) == "yuv420sp":
        format = "NV21";
    elif m.group(4) == "yuv420p":
        format = "YV12";
    elif m.group(4) == "rgb565":
        format = "RGB565"
    else:
        sys.exit();
    raw_to_bmp(sys.argv[1], width, height, format)


if __name__ =="__main__":
    main();


