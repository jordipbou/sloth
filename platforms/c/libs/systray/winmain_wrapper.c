#ifdef _WIN32
#include <windows.h>

/* This wrapper allows including windows.h here without */
/* conflicting with raylib.h that is included in example.c */

// Forward declaration of your normal main()
int main(void);

// WinMain forwards to main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    return main();
}
#endif
