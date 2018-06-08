This software was originally written for the purpose of getting rid of black borders when watching movies, but may also be useful for different tasks.

For example, for 21:9 movies on a 21:9 monitor, Netflix shows (or rather: used to show) black borders on all four edges of the screen, effectively reducing the screen size dramatically.

If you want to compile/run the program, you need to have OpenCV installed (successfully tested with version 2.4.9): https://opencv.org/

If you just want to run it without having to do a full OpenCV install, you just need the following *.dll files from OpenCV (which I am, probably, not allowed to attach due to license issues):
opencv_imgproc249.dll
opencv_highgui249.dll
opencv_core249.dll

Controls:
* : zoom in
/ : zoom out
+ : zoom in slightly (useful for fine-tuning)
- : zoom out slightly (useful for fine-tuning)
i : turn interlacing on/off
r : reset zoom factor
c : center zoom window
num pad 8,4,5,6 : move zoom window
w,a,s,d : move zoom window slightly (useful for fine-tuning)
any other key: start/pause/resume zooming

Known problems:
- The mouse does not work while zoomed in
- The window with the zoom button is sometimes "in the way". Solution: just drag it to whichever area on the screen you want to get rid of by zooming (e.g., black movie borders).
- For some fullscreen apps, zooming just shows gibberish / black screen (e.g. Netflix app). Netflix workaround: stream Netflix from browser instead of windows app.
- The frame rate may be (slightly) unsteady, which can result in micro lags, especially when using interlacing. Some people do not even notice this, so you need to experiment. To get rid of micro lags, turn off interlacing, especially if you use a high resolution (4k+) monitor, where interlacing is not as important for image quality.