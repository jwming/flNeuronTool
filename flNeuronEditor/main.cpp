#include "window.h"

#include <stdio.h>
#include <FL/Fl.h>
#include <FL/Fl_Ask.h>

#ifdef _DEBUG
#pragma comment(lib, "libjpegd.lib")
#pragma comment(lib, "libtiffd.lib")
#pragma comment(lib, "fltkd.lib")
#pragma comment(lib, "fltkgld.lib")
#else
//#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#pragma comment(lib, "libjpeg.lib")
#pragma comment(lib, "libtiff.lib")
#pragma comment(lib, "fltk.lib")
#pragma comment(lib, "fltkgl.lib")
#endif
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glew32.lib")

int main(int argc, char **argv)
{
    printf("flNeuronTool Editor\
           \nA Fast Light Neuron Editor Tool\
           \nversion 1.0\
           \nmingxing@hust.edu.cn\n");
    Window window(800, 800+25, "flNeuronTool Editor");
    window.show(argc, argv);
    fl_message_title_default("flNeuronTool Editor");
    return Fl::run();
}
