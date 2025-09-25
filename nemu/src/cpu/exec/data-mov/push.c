#include "cpu/exec/helper.h"

// 实例化不同长度的版本
#define DATA_BYTE 1
#include "push-template.h"

#define DATA_BYTE 2
#include "push-template.h"

#define DATA_BYTE 4
#include "push-template.h"

make_helper_v(push_r);  