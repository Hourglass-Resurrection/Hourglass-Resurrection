Some behavior is currently incorrect (like `GetKey[board]State()` changing immediately while it should only change when
the game reads the corresponding `WM_<...>` messages). Change the test output log when this behavior is corrected.

Code used to generate `test.hgr`:
```cpp
Movie movie;
movie.fps = 60;
movie.it = 6000;

CurrentInput frame{};
movie.frames.push_back(MovieFrame{ frame });

frame.keys['Q'] = 1;
movie.frames.push_back(MovieFrame{ frame });

frame.keys['Q'] = 0;
movie.frames.push_back(MovieFrame{ frame });

frame.keys['A'] = 1;
frame.keys['S'] = 1;
frame.keys['D'] = 1;
movie.frames.push_back(MovieFrame{ frame });

frame.keys['A'] = 0;
frame.keys['S'] = 0;
movie.frames.push_back(MovieFrame{ frame });

frame.keys['D'] = 0;
movie.frames.push_back(MovieFrame{ frame });

frame.keys[VK_F1] = 1;
movie.frames.push_back(MovieFrame{ frame });

frame.keys[VK_F1] = 0;
frame.keys[VK_CAPITAL] = 1;
movie.frames.push_back(MovieFrame{ frame });

frame.keys[VK_CAPITAL] = 0;
movie.frames.push_back(MovieFrame{ frame });

frame.mouse.coords.x = 10;
frame.mouse.coords.y = 20;
frame.mouse.di.lX = 10;
frame.mouse.di.lY = 20;
frame.mouse.di.rgbButtons[0] = MOUSE_PRESSED_FLAG;
frame.mouse.di.rgbButtons[1] = MOUSE_PRESSED_FLAG;
frame.mouse.di.rgbButtons[2] = MOUSE_PRESSED_FLAG;
movie.frames.push_back(MovieFrame{ frame });

SaveMovieToFile(movie, L"test.hgr");
```