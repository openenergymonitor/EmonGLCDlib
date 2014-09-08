enum {
    REMOTE_GLCD_CLEAR,          // 0 params
    REMOTE_GLCD_SETPIXEL,       // 3 params: x,y,color
    REMOTE_GLCD_DRAWLINE,       // 5 params: x1,y1,x2,y2,color
    REMOTE_GLCD_DRAWRECT,       // 5 params: x1,y1,x2,y2,color
    REMOTE_GLCD_FILLRECT,       // 5 params: x1,y1,x2,y2,color
    REMOTE_GLCD_DRAWCIRCLE,     // 4 params: x,y,r,color
    REMOTE_GLCD_FILLCIRCLE,     // 4 params: x,y,r,color
    REMOTE_GLCD_DRAWTRIANGLE,   // 7 params: x1,y1,x2,y2,x3,y3,color
    REMOTE_GLCD_FILLTRIANGLE,   // 7 params: x1,y1,x2,y2,x3,y3,color
    REMOTE_GLCD_DRAWCHAR,       // 3 params: x,y,char
    REMOTE_GLCD_DRAWSTRING,     // 3+ x,y,str.....,\0
    REMOTE_GLCD_UPDATEAREA,     // 5 params: x1,y1,x2,y2,reset(0/1)
    REMOTE_GLCD_SETUPDATEAREA,  // 5 params: x1,y1,x2,y2,shrink(0/1)
    REMOTE_GLCD_DRAWBMP,        // 5 + BMP data
    REMOTE_GLCD_REFRESH,        // 0 params
    REMOTE_GLCD_SCROLL,         // 2 params: dir,pixels
    REMOTE_GLCD_BACKLIGHT,      // 1 params: level
};
