#include <windows.h>

int main(int, char**);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
    int argc = 0;
    char** argv = nullptr;
    return main(argc, argv);
}