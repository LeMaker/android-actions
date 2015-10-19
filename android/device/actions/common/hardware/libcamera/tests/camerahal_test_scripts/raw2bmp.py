

import sys
import os
import struct
import getopt

DEFAULT_STRIDE = -1
DEFAULT_FRAME = 0
DEFAULT_FORMAT = "NV21"

def o16(i):
    return chr(i&255) + chr(i>>8&255)

def o32(i):
    return chr(i&255) + chr(i>>8&255) + chr(i>>16&255) + chr(i>>24&255)

def color_limit(c):
    if c >= 256:
        return 255;
    if c <= 0:
        return 0
    return int(c);

class Converter(object):
    """Base class that should define interface for all conversions"""
    def __init__(self, filename, width, height, stride, frame):
        self.filename = filename
        self.width = width
        self.height = height
        self.stride = stride
        self.frame = frame

        self.pixels = [[[0 for pix in range(3)] for col in range(self.width)] for row in range(self.height)];

    def Convert():
        raise NotImplementedError( "Should have implemented this!" )


    def save_to_bmp(self, filename):
        fp = open(filename, "wb")
        bits = 24
        stride = ((self.width*bits+7)/8+3)&(~3)
        header = 108 # or 64 for OS/2 version 2
        offset = 14 + header + 0 * 4
        image  = stride * self.height

        compress = 0;

        # bitmap header
        fp.write("BM" +                     # file type (magic)
                 o32(offset+image) +        # file size
                 o32(0) +                   # reserved
                 o32(offset))               # image data offset

        # bitmap info header
        fp.write(o32(header) +              # info header size
                 o32(self.width) +          # width
                 o32(-self.height) +         # height
                 o16(1) +                   # planes
                 o16(bits) +                # depth
                 o32(compress) +            # compression (0=uncompressed)
                 o32(0) +                   # size of bitmap
                 o32(0x0b12) + o32(0x0b12) +          # resolution
                 o32(0) +              # colors used
                 o32(0))               # colors important

        fp.write("\000" * (header - 40 ))    # padding (for OS/2 format)

        row_fmt = 'B'*self.width*3+'x'*(stride-self.width*3);
        data = "";
        for row in self.pixels:
            row_list = []
            for pix in row:
                for c in pix[::-1]:
                    row_list.append(c);    

            data += struct.pack(row_fmt, *row_list)
        fp.write(data);  
        fp.close();



class NV12Converter(Converter):
    """This class converts NV12 files into RGB"""
    def __init__(self, filename, width, height, stride, frame):
        super(NV12Converter, self).__init__(filename, width, height, stride, frame)

    def Convert(self):
        f = open(self.filename, "rb")

        converted_image_filename = self.filename.split('.')[0] + ".bmp"

        size_of_file = os.path.getsize(self.filename)
        size_of_frame = ((self.height*self.width*3)//2)
        number_of_frames = size_of_file // size_of_frame
        frame_start = size_of_frame * self.frame

        f.seek(frame_start);        
        data = f.read(int(size_of_frame));
        y_start = 0;
        uv_start = (self.width*self.height)
        y_index = 0;
        for j in range(0, self.height):
            for i in range(0, self.width):
                uv_index = int(uv_start + (self.width * (j//2)) + ((i//2))*2)

                y = ord(data[y_index])
                u = ord(data[uv_index])
                v = ord(data[uv_index+1])

                b = 1.164 * (y-16) + 2.018 * (u - 128)
                g = 1.164 * (y-16) - 0.813 * (v - 128) - 0.391 * (u - 128)
                r = 1.164 * (y-16) + 1.596*(v - 128)

                self.pixels[j][i] = [color_limit(r), color_limit(g), color_limit(b)]
                y_index = y_index+1;


        self.save_to_bmp(converted_image_filename);


class NV21Converter(Converter):
    """This class converts NV21 files into RGB"""
    def __init__(self, filename, width, height, stride, frame):
        super(NV21Converter, self).__init__(filename, width, height, stride, frame)

    def Convert(self):
        f = open(self.filename, "rb")

        converted_image_filename = self.filename.split('.')[0] + ".bmp"

        size_of_file = os.path.getsize(self.filename)
        size_of_frame = ((self.height*self.width*3)//2)
        number_of_frames = size_of_file // size_of_frame
        frame_start = size_of_frame * self.frame

        f.seek(frame_start);        
        data = f.read(int(size_of_frame));
        y_start = 0;
        uv_start = (self.width*self.height)
        y_index = 0;
        for j in range(0, self.height):
            for i in range(0, self.width):
                uv_index = int(uv_start + (self.width * (j//2)) + (i//2)*2)

                y = ord(data[y_index])
                v = ord(data[uv_index])
                u = ord(data[uv_index+1])

                b = 1.164 * (y-16) + 2.018 * (u - 128)
                g = 1.164 * (y-16) - 0.813 * (v - 128) - 0.391 * (u - 128)
                r = 1.164 * (y-16) + 1.596*(v - 128)

                self.pixels[j][i] = [color_limit(r), color_limit(g), color_limit(b)]
                y_index=y_index+1;


        self.save_to_bmp(converted_image_filename);


class YU12Converter(Converter):
    """This class converts YU12 files into RGB"""
    def __init__(self, filename, width, height, stride, frame):
        super(YU12Converter, self).__init__(filename, width, height, stride, frame)

    def Convert(self):
        f = open(self.filename, "rb")

        converted_image_filename = self.filename.split('.')[0] + ".bmp"

        size_of_file = os.path.getsize(self.filename)
        size_of_frame = ((self.height*self.width*3)//2)
        number_of_frames = size_of_file // size_of_frame
        frame_start = size_of_frame * self.frame

        f.seek(frame_start);        
        data = f.read(int(size_of_frame));
        y_start = 0;
        u_start = (self.width*self.height)
        v_start = u_start + int((self.width*self.height)//4)
        y_index = y_start;
        u_index = 0;
        v_index = 0;
        for j in range(0, self.height):
            u_index = int(u_start + (self.width//2) * (j//2));
            v_index = int(v_start + (self.width//2) * (j//2));
            for i in range(0, self.width):
                y = ord(data[y_index])
                u = ord(data[u_index])
                v = ord(data[v_index])

                b = 1.164 * (y-16) + 2.018 * (u - 128)
                g = 1.164 * (y-16) - 0.813 * (v - 128) - 0.391 * (u - 128)
                r = 1.164 * (y-16) + 1.596*(v - 128)

                self.pixels[j][i] = [color_limit(r), color_limit(g), color_limit(b)]
                y_index += 1;
                if(i%2 == 1):
                    u_index += 1;
                    v_index += 1;


        self.save_to_bmp(converted_image_filename);

class YV12Converter(Converter):
    """This class converts YV21 files into RGB"""
    def __init__(self, filename, width, height, stride, frame):
        super(YV12Converter, self).__init__(filename, width, height, stride, frame)

    def Convert(self):
        f = open(self.filename, "rb")

        converted_image_filename = self.filename.split('.')[0] + ".bmp"

        size_of_file = os.path.getsize(self.filename)
        size_of_frame = ((self.height*self.width*3)//2)
        number_of_frames = size_of_file // size_of_frame
        frame_start = size_of_frame * self.frame

        f.seek(frame_start);        
        data = f.read(int(size_of_frame));
        y_start = 0;
        v_start = (self.width*self.height)
        u_start = v_start + int((self.width*self.height)//4) 
        y_index = y_start;
        u_index = 0;
        v_index = 0;
        for j in range(0, self.height):
            u_index = int(u_start + (self.width//2) * (j//2));
            v_index = int(v_start + (self.width//2) * (j//2));
            for i in range(0, self.width):
                y = ord(data[y_index])
                u = ord(data[u_index])
                v = ord(data[v_index])

                b = 1.164 * (y-16) + 2.018 * (u - 128)
                g = 1.164 * (y-16) - 0.813 * (v - 128) - 0.391 * (u - 128)
                r = 1.164 * (y-16) + 1.596*(v - 128)

                self.pixels[j][i] = [color_limit(r), color_limit(g), color_limit(b)]
                y_index += 1;
                if(i%2 == 1):
                    u_index += 1;
                    v_index += 1;


        self.save_to_bmp(converted_image_filename);

class RGB565Converter(Converter):
    """This class converts RGB565 files into RGB"""
    def __init__(self, filename, width, height, stride, frame):
        super(RGB565Converter, self).__init__(filename, width, height, stride, frame)
    #constructor#

    def Convert(self):
        f = open(self.filename, "rb")

        converted_image_filename = self.filename.split('.')[0] + ".bmp"

        size_of_file = os.path.getsize(self.filename)
        size_of_frame = ((self.height*self.width*2))
        number_of_frames = size_of_file // size_of_frame
        frame_start = size_of_frame * self.frame

        f.seek(frame_start);        
        data = f.read(int(size_of_frame));
        for j in range(0, self.height):
            for i in range(0, self.width):
                position = (j*self.width + i)*2 
                b = ord(data[position+1])&0xf8
                g = ((ord(data[position+1])&0x7)<<5) | ((ord(data[position])&0xe0)>>3)
                r = (ord(data[position])&0x1f)<<3

                self.pixels[j][i] = [color_limit(r), color_limit(g), color_limit(b)]


        self.save_to_bmp(converted_image_filename);


class RGB888Converter(Converter):
    """This class converts RGB565 files into RGB"""
    def __init__(self, filename, width, height, stride, frame):
        super(RGB888Converter, self).__init__(filename, width, height, stride, frame)


    def Convert(self):
        f = open(self.filename, "rb")

        self.bytes_per_row = self.width*2;

        converted_image_filename = self.filename.split('.')[0] + ".bmp"

        size_of_file = os.path.getsize(self.filename)
        size_of_frame = ((self.height*self.width*3))
        number_of_frames = size_of_file // size_of_frame
        frame_start = size_of_frame * self.frame

        f.seek(frame_start);        
        data = f.read(int(size_of_frame));
        for j in range(0, self.height):
            for i in range(0, self.width):
                position = (j*self.width + i)*3 
                r = ord(data[position])
                g = ord(data[position+1])
                b = ord(data[position+2])

                self.pixels[j][i] = [color_limit(r), color_limit(g), color_limit(b)]
       

        self.save_to_bmp(converted_image_filename);


def printUsage():
    print
    print "  usage: " + sys.argv[0] + " [options] filename width height format "
    print
   

def raw_to_bmp(filename, width, height, format="NV21", stride=-1, frame=0):
    if format == "NV12":
        converter = NV12Converter(filename, width, height, stride, frame)
    elif format == "NV21":
        converter = NV21Converter(filename, width, height, stride, frame)
    elif format == "YU12":
        converter = YU12Converter(filename, width, height, stride, frame)
    elif format == "YV12":
        converter = YV12Converter(filename, width, height, stride, frame)
    elif format == "RGB565":
        converter = RGB565Converter(filename, width, height, stride, frame)
    elif format == "RGB888":
        converter = RGB888Converter(filename, width, height, stride, frame)
    else:
        raise Expception("format not support");

    converter.Convert()

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "s:f:", ["stride=", "frame="])
    except getopt.GetoptError, err:
        print str(err) # will print something like "option -a not recognized"
        printUsage()
        sys.exit();
    
    stride = DEFAULT_STRIDE;
    frame =DEFAULT_FRAME;
    width = 0;
    height = 0;
    format= DEFAULT_FORMAT;
    filename = "";
    try:
        for o, a in opts:
            if o in ("-h", "--help"):
                printUsage()
                sys.exit()
            elif o in ("-s", "--stride"):
                stride = a
            elif f in ("-f", "--frame"):
                frame = int(a)
            else:
                assert False, "unhandled option"

        if stride == DEFAULT_STRIDE:
            stride = width

        if frame < 0:
            frame = 0

        if len(args) == 3:
            filename = args[0]; 
            width = int(args[1]);
            height = int(args[2]);
        elif len(args) == 4:
            filename = args[0]; 
            width = int(args[1]);
            height = int(args[2]);
            format = args[3];
        else:
            printUsage()
            sys.exit();
    except:
        printUsage()
        sys.exit();




    try:
        raw_to_bmp(filename, width, height, format,stride,frame);
    except:
        printUsage()
        sys.exit();



#main function#

if __name__ == "__main__":
    main()
