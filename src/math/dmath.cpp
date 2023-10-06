#include "dmath.h"

#include <math.h>
#include <stdlib.h>

static b8 randSeeded = false;




//i32 drandom(){
//    if(!randSeeded){
//        srand((u32)PlatformGetAbsoluteTime());
//        randSeeded = true;
//    }
//    return rand();
//}
//
//i32 drandomInRange(i32 min, i32 max){
//    if(!randSeeded){
//        srand((u32)PlatformGetAbsoluteTime());
//        randSeeded = true;
//    }
//    return (rand() % (max - min + 1)) + min;
//}

//f32 drandomF(){
//    return (f32)drandom() / (f32)RAND_MAX;
//}
//
//f32 drandomFInRange(f32 min, f32 max){
//    return min + ((f32)drandom() / ((f32)RAND_MAX / (max - min)));
//}