#include <WindowManager.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>

WindowManager::WindowManager(DoubleBuffer &doubleBuffer) : doubleBuffer(doubleBuffer), width(doubleBuffer.getWidth()), height(doubleBuffer.getHeight()), keysPressed(0), running(false)
{
    if (!(display = XOpenDisplay(NULL)))
        throw std::runtime_error("Cannot connect to X server");

    imgBuffer = (int *)malloc(width * height * sizeof(int));
    memset(imgBuffer, 0, width * height * sizeof(int));

    screen = DefaultScreen(display);

    unsigned long black = BlackPixel(display, screen);

    window = XCreateSimpleWindow(display,
                                 RootWindow(display, screen),
                                 0, 0,
                                 width, height,
                                 0,
                                 black,
                                 black);

    XSelectInput(display, window, StructureNotifyMask | KeyPressMask | KeyReleaseMask);
    XMapWindow(display, window);
    gc = XCreateGC(display, window, 0, NULL);

    for (;;)
    {
        XEvent e;
        XNextEvent(display, &e);
        if (e.type == MapNotify)
            break;
    }

    Visual *visual = DefaultVisual(display, screen);
    img = XCreateImage(display,
                       visual,
                       DefaultDepth(display, screen),
                       ZPixmap,
                       0,
                       (char *)imgBuffer,
                       width, height,
                       32,
                       0);
    if (!img)
        throw std::runtime_error("Cannot create image");
}

WindowManager::~WindowManager()
{
    stopWindowThread();
    XDestroyImage(img);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

void WindowManager::startWindowThread()
{
    // If the thread is not running, we start it
    if (!running)
    {
        running = true;
        windowThread = std::thread(&WindowManager::windowThreadFunction, this);
    }
}

void WindowManager::stopWindowThread()
{   
    // If the thread is not running, we don't need to stop it
    if (running)
    {
        running = false;

        // If the thread is joinable, we join it
        if (windowThread.joinable())
        {
            windowThread.join();
        }
    }
}

void WindowManager::windowThreadFunction()
{
    while (running)
    {
        // Update the display to show the current frame
        updateDisplay();

        // Update the input
        updateInput();

        // Sleep for 10 milliseconds to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

unsigned int WindowManager::getKeysPressed() { return keysPressed; }

void WindowManager::updateDisplay()
{
    const std::vector<int> &backBuffer = doubleBuffer.getBackBuffer();
    std::copy(backBuffer.begin(), backBuffer.end(), imgBuffer);

    XPutImage(display, window, gc, img, 0, 0, 0, 0, width, height);
}

void WindowManager::updateInput()
{
    XEvent e;
    while (XPending(display))
    {
        XNextEvent(display, &e);
        switch (e.type)
        {
        case KeyPress:
            keysPressed |= convertKey(XLookupKeysym(&e.xkey, 0));
            break;
        case KeyRelease:
            keysPressed &= ~convertKey(XLookupKeysym(&e.xkey, 0));
            break;
        }
    }
}

unsigned int WindowManager::convertKey(KeySym key)
{
    switch (key)
    {
    case XK_Up:
        return KEY_UP;
    case XK_Down:
        return KEY_DOWN;
    case XK_Left:
        return KEY_LEFT;
    case XK_Right:
        return KEY_RIGHT;
    case XK_Escape:
        return KEY_ESC;
    }
    return 0;
}