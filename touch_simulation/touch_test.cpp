//
// Created by Administrator on 2024/10/2.
//


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <map>

enum FingerStatus {
    FINGER_NO, // 无状态
    FINGER_X_UPDATE, // X更新
    FINGER_Y_UPDATE, // Y更新
    FINGER_XY_UPDATE, // XY同时更新
    FINGER_UP, // 抬起
    FINGER_DOWN, // 按下
};

struct TouchFinger {
    int x = -1, y = -1; // 触摸点XY数据
    int tracking_id = -1; // 触摸点追踪ID数据
    int status = FINGER_NO;
    timeval time;
};// 10根手指

struct Device {
    int fd;
    float S2TX;
    float S2TY;
    input_absinfo absX, absY;
    TouchFinger Finger[10];
    Device() { memset((void *) this, 0, sizeof(*this)); }
};

Device device;
struct DisplayInfo {
    int32_t orientation;
    int32_t width;
    int32_t height;
};

DisplayInfo displayInfo;

float x_proportion, y_proportion;
int ScreenType = 0; // 竖屏 横屏



static bool checkDeviceIsTouch(int fd) {
    uint8_t *bits = NULL;
    ssize_t bits_size = 0;
    int res, j, k;
    bool itmp = false, itmp2 = false, itmp3 = false;
    struct input_absinfo abs{};
    while (true) {
        res = ioctl(fd, EVIOCGBIT(EV_ABS, bits_size), bits);
        if (res < bits_size)
            break;
        bits_size = res + 16;
        bits = (uint8_t *) realloc(bits, bits_size * 2);
    }
    for (j = 0; j < res; j++) {
        for (k = 0; k < 8; k++)
            if (bits[j] & 1 << k && ioctl(fd, EVIOCGABS(j * 8 + k), &abs) == 0) {
                if (j * 8 + k == ABS_MT_SLOT) {
                    itmp = true;
                    continue;
                }
                if (j * 8 + k == ABS_MT_POSITION_X) {
                    itmp2 = true;
                    continue;
                }
                if (j * 8 + k == ABS_MT_POSITION_Y) {
                    itmp3 = true;
                    continue;
                }
            }
    }
    free(bits);
    return itmp && itmp2 && itmp3;
}
// 获取触控设备ID
bool GetEventId()
{
    DIR *dir = opendir("/dev/input/");
    if (!dir) {
        return false;
    }

    dirent *ptr = NULL;
    int eventCount = 0;
    while ((ptr = readdir(dir)) != NULL) {
        if (strstr(ptr->d_name, "event"))
            eventCount++;
    }

    char temp[128];
    for (int i = 0; i <= eventCount; i++) {
        sprintf(temp, "/dev/input/event%d", i);
        int fd = open(temp, O_RDWR);
        if (fd < 0) {
            continue;
        }
        // 判断是否是触控设备
        if (checkDeviceIsTouch(fd)) {
            char devicepath[64];
            sprintf(devicepath, "/dev/input/event%d", i);
            printf("Eventpath: %s \n", devicepath);

            if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &device.absX) == 0
                && ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &device.absY) == 0) {
                device.fd = fd;

                struct input_absinfo absX;
                struct input_absinfo absY;
                ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX);
                ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY);
                float Width = absX.maximum + 1;
                float Height = absY.maximum + 1;
                //计算缩放比例
                int scr_x = displayInfo.width;
                int scr_y = displayInfo.height;

                if(scr_x > scr_y){
                    int t = scr_y;
                    scr_y = scr_x;
                    scr_x = t;
                }
                x_proportion = Width / scr_x;
                y_proportion = Height / scr_y;

                return true;
            }
        } else {
            close(fd);
        }
    }
    return false;
}

int last_slot = 0;
bool touch_status = false;

static int device_writeEvent(int fd, uint16_t type, uint16_t keycode, int32_t value) {
    struct input_event ev;

    memset(&ev, 0, sizeof(struct input_event));

    ev.type = type;
    ev.code = keycode;
    ev.value = value;
    if (write(fd, &ev, sizeof(struct input_event)) < 0) {
        perror("write error");
        return -1;
    }
    return 1;
}

void device_close() {
    if (device.fd > 0) {
        close(device.fd);
        device.fd = -1;
    }
}

/*
void Upload() {
    if (dev_fd <= 0) {
        return;
    }
    int32_t last_tmpCnt = 0;
    int32_t tmpCnt = 0;
    struct input_event event[128];

    for (int i = 0; i < 10; i ++) {
        auto& Finger = Fingers[i];
        if (Fingers[i].status == FINGER_NO) {
            continue;
        }

        if (Finger.status != FINGER_UP) {
            if (!touch_status)
            {
                touch_status = true;
                event[tmpCnt].type = EV_KEY;
                event[tmpCnt].code = BTN_TOUCH;
                event[tmpCnt].value = 1;
                tmpCnt++;
            }

            if (last_slot != i)
            {
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_SLOT;
                event[tmpCnt].value = i;
                tmpCnt++;
                last_slot = i;
            }

            event[tmpCnt].type = EV_ABS;
            event[tmpCnt].code = ABS_MT_TRACKING_ID;
            event[tmpCnt].value = Finger.tracking_id;
            tmpCnt++;

            bool x_update = Finger.status == FINGER_X_UPDATE || Finger.status == FINGER_XY_UPDATE;
            bool y_update = Finger.status == FINGER_Y_UPDATE || Finger.status == FINGER_XY_UPDATE;

            if (x_update) {
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_POSITION_X;
                event[tmpCnt].value = Finger.x;
                tmpCnt++;
            }
            if (y_update) {
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_POSITION_Y;
                event[tmpCnt].value = Finger.y;
                tmpCnt++;
            }

            event[tmpCnt].type = EV_SYN;
            event[tmpCnt].code = SYN_REPORT;
            event[tmpCnt].value = 0;
            tmpCnt++;
        } else {
            if (last_slot != i)
            {
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_SLOT;
                event[tmpCnt].value = i;
                tmpCnt++;
                last_slot = i;
            }

            event[tmpCnt].type = EV_ABS;
            event[tmpCnt].code = ABS_MT_TRACKING_ID;
            event[tmpCnt].value = -1;
            tmpCnt++;

            event[tmpCnt].type = EV_SYN;
            event[tmpCnt].code = SYN_REPORT;
            event[tmpCnt].value = 0;
            tmpCnt++;

            touch_status = false;
        }

        for (int j = last_tmpCnt; j < tmpCnt; j ++) {
            event[j].time = Finger.time;
        }

        Finger.status = FINGER_NO;
        last_tmpCnt = tmpCnt;
    }

    write(dev_fd, event, sizeof(struct input_event) * tmpCnt);
}
*/

void slot_Upload(int slot) {
    if (device.fd <= 0) {
        perror("device not found");
        return;
    }
    //printf("slot_Upload: slot:%d  lst_slot:%d\n", slot, last_slot);
    //获取手指数据
    auto& Finger = device.Finger[slot];
    if (Finger.status == FINGER_NO) {
        return;
    }

    struct input_event event[16];
    // 开始上传数据

    int32_t tmpCnt = 0;
    // 重置事件
    //非抬起状态
    if (Finger.status != FINGER_UP) {
        //如果没有触摸，则需要更新触摸状态
        if (!touch_status)
        {
            touch_status = true;
            event[tmpCnt].type = EV_KEY;
            event[tmpCnt].code = BTN_TOUCH;
            event[tmpCnt].value = 1;
            tmpCnt++;
            event[tmpCnt].type = EV_KEY;
            event[tmpCnt].code = BTN_TOOL_FINGER;
            event[tmpCnt].value = 1;
            tmpCnt++;

        }
        //如果上一次的slot和当前的slot不一样，则需要更新slot
        //要一直更新slot，不然抢触摸事件
       // if (last_slot != slot)
       // {
            event[tmpCnt].type = EV_ABS;
            event[tmpCnt].code = ABS_MT_SLOT;
            event[tmpCnt].value = slot;
            tmpCnt++;
            last_slot = slot;
       // }
        //更新tracking_id
        event[tmpCnt].type = EV_ABS;
        event[tmpCnt].code = ABS_MT_TRACKING_ID;
        event[tmpCnt].value = Finger.tracking_id;
        tmpCnt++;
        //更新坐标
        bool x_update = Finger.status == FINGER_X_UPDATE || Finger.status == FINGER_XY_UPDATE;
        bool y_update = Finger.status == FINGER_Y_UPDATE || Finger.status == FINGER_XY_UPDATE;

        if (x_update) {
            event[tmpCnt].type = EV_ABS;
            event[tmpCnt].code = ABS_MT_POSITION_X;
            event[tmpCnt].value = Finger.x;
            tmpCnt++;
        }
        if (y_update) {
            event[tmpCnt].type = EV_ABS;
            event[tmpCnt].code = ABS_MT_POSITION_Y;
            event[tmpCnt].value = Finger.y;
            tmpCnt++;
        }

        event[tmpCnt].type = EV_SYN;
        event[tmpCnt].code = SYN_REPORT;
        event[tmpCnt].value = 0;
        tmpCnt++;
    }
    else {
        //按下状态->抬起
        //如果上一次的slot和当前的slot不一样，则需要更新slot
       // if (last_slot != slot)
        //{
            event[tmpCnt].type = EV_ABS;
            event[tmpCnt].code = ABS_MT_SLOT;
            event[tmpCnt].value = slot;
            tmpCnt++;
            last_slot = slot;
       //}
        //更新tracking_id 为-1 表示抬起
        event[tmpCnt].type = EV_ABS;
        event[tmpCnt].code = ABS_MT_TRACKING_ID;
        event[tmpCnt].value = -1;
        tmpCnt++;
        //更新事件
        event[tmpCnt].type = EV_SYN;
        event[tmpCnt].code = SYN_REPORT;
        //SYN_MT_REPORT 只上报多点触控，会残留触摸点
        event[tmpCnt].value = 0;
        tmpCnt++;

        touch_status = false;
    }

    for (int i = 0; i < tmpCnt; i ++) {
        event[i].time = Finger.time;
    }

    Finger.status = FINGER_NO;

    write(device.fd, event, sizeof(struct input_event) * tmpCnt);
}


//自瞄的手指
int aimslot = 0;


/*
 * 触摸点都是竖屏下的坐标点
 * */
void Touch_Down(int slot, int x,int y){

    aimslot = slot;
    auto& Finger = device.Finger[slot];
    //判断手指状态
    if (Finger.x != x && Finger.y != y)
        Finger.status = FINGER_XY_UPDATE;
    else if (Finger.x != x)
        Finger.status = FINGER_X_UPDATE;
    else if (Finger.y != y)
        Finger.status = FINGER_Y_UPDATE;

   // printf("Touch_Down: slot:%d, x:%d, y:%d\n", slot, x, y);
   // printf("Touch_Down: status:%d\n", Finger.status);

    Finger.x = x * x_proportion;
    Finger.y = y * y_proportion;
    Finger.tracking_id = slot;
    gettimeofday(&Finger.time, 0);
    //上传数据
    slot_Upload(slot);




}

void Touch_Move(int slot, int x,int y){
    Touch_Down(slot, x, y);
}

void Touch_Up(int slot){
    aimslot = slot;
    auto& Finger = device.Finger[slot];
    Finger.status = FINGER_UP;
    gettimeofday(&Finger.time, 0);
    //上传数据
    slot_Upload(slot);
   // printf("Touch_Up: slot:%d\n", slot);
}

//模拟触摸事件=================

// 使用缓动函数模拟加速-减速滑动
float easing_function(float t) {
    return t * t * (3 - 2 * t);  // 贝塞尔缓动曲线，模拟加速-减速
}

// 根据距离计算适当的步数和持续时间
void calculate_steps_and_duration(int start_x, int start_y, int end_x, int end_y, int *steps, int *duration_ms) {
    // 计算滑动距离
    int distance = sqrt(pow(end_x - start_x, 2) + pow(end_y - start_y, 2));

    // 动态调整步数和持续时间，步数和时间正比于距离
    *steps = distance / 10;   // 每10像素对应1步，可以根据需要调整
    *steps = (*steps < 10) ? 10 : *steps; // 最少10步
    *duration_ms = distance * 2;  // 每像素2毫秒，可以根据需要调整
    *duration_ms = (*duration_ms < 200) ? 200 : *duration_ms; // 最短200ms
}

// 在每步滑动时加入微小随机偏移，模拟手指自然抖动
int add_random_jitter(int value, int jitter) {
    return value + (rand() % (2 * jitter + 1)) - jitter;
}

// 模拟滑动操作，添加时间戳、加速度、减速度及随机抖动
void simulate_swipe(int fd, int slot, int start_x, int start_y, int end_x, int end_y) {
    // 计算动态步数和持续时间
    int steps, duration_ms;
    calculate_steps_and_duration(start_x, start_y, end_x, end_y, &steps, &duration_ms);

    // 手指按下 (滑动起点)
    Touch_Down(slot, start_x, start_y);

    // 滑动过程
    int jitter = 3;  // 加入随机抖动，模拟手指自然抖动
    float delta_x = (float)(end_x - start_x);
    float delta_y = (float)(end_y - start_y);

    for (int i = 1; i <= steps; i++) {
        // 使用缓动函数计算当前时间进度
        float t = (float)i / steps;
        float eased_t = easing_function(t);

        // 根据缓动后的时间计算新的坐标
        int x = start_x + (int)(eased_t * delta_x);
        int y = start_y + (int)(eased_t * delta_y);

        // 加入微小的随机抖动
        x = add_random_jitter(x, jitter);
        y = add_random_jitter(y, jitter);

        Touch_Move(slot, x, y);

        // 控制时间间隔
        usleep((duration_ms / steps) * 1000);              // 每一步的时间间隔
    }

    // 手指松开 (滑动结束)
    Touch_Up(slot);
}

#define maxF 10

int main()
{
    //命令获取屏幕分辨率 umi:/dev/input # wm size
    //Physical size: 1080x2340
    char cmd[128];
    sprintf(cmd, "wm size | awk '{print $3}'");
    FILE *fp = popen(cmd, "r");
    printf("cmd: %s\n", cmd);
    fscanf(fp, "%dx%d", &displayInfo.width, &displayInfo.height);
    pclose(fp);
    printf("Screen resolution: %dx%d\n", displayInfo.width, displayInfo.height);
    //初始化设备
    if (GetEventId() == false){
        printf("No touch device found!\n");
        return 0;
    }
    input_event inputEvent[64];
    int slotMap[10] = {0};
/*
    int tt = 0;
    while (tt++<500) {
        memset(inputEvent, 0, sizeof(inputEvent));
        //读数据需要触摸一下屏幕
        auto readSize = (int32_t) read(device.fd, inputEvent, sizeof(inputEvent));
        if (readSize <= 0 || (readSize % sizeof(input_event)) != 0) {
            printf("read error\n");
            return 0;
        }

        size_t count = size_t(readSize) / sizeof(input_event);

        for (size_t j = 0; j < count; j++) {
            input_event &ie = inputEvent[j];
            if (ie.type == EV_ABS) {
                //获取最近的slot
                if (ie.code == ABS_MT_SLOT) {
                    printf("slot: %d\n", ie.value);
                    //latest = ie.value;
                    slotMap[ie.value] = 1;
                    continue;
                }
                if (ie.code == ABS_MT_TRACKING_ID) {
                    printf("tracking_id: %d\n", ie.value);
//                if (ie.value == -1) {
//                    device.Finger[latest].status = FINGER_NO;
//                } else {
//                    device.Finger[latest].tracking_id =  latest;
//                    device.Finger[latest].status = FINGER_DOWN;
//                }
                    continue;
                }
            }

        }
*/

        // }
    //遍历slotMap，获取下一个slot
    //slot是从0开始的，一般都是一根手指在触摸屏幕，
    //每多一个手指触摸屏幕，slot就加1，
    // 这里从1开始，（直接write的好像与触摸屏的记录点不同，所以
    //存在bug，手指更新不及时，导致跳触摸点，只能是最外层的手指才能准确
    // 解决方法：1.像原版那样，线程循环读取数据，更新绑定的手指slot
    //测试 直接4/5/6随便
    int freeSlot = -1;
    if(slotMap[0] == 0) {
        freeSlot = 1;
    }
    for (int i = 1; i < 10; i++) {

        if (slotMap[i] == 0) {
            freeSlot = i;
            break;
        }
    }
    printf("freeSlot: %d\n", freeSlot);

    //模拟触摸事件
    simulate_swipe(device.fd, 5, 400, 400, 1000, 1000);

    //释放设备
    device_close();


    return 0;
}
