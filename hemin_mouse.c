#include <reg52.h> //头文件
#include <intrins.h>
typedef unsigned char uchar; // 宏定义
typedef unsigned int uint;
/*将数据定义在程序存储器需要在C51扩展的关键字code进行说明，
对程序存储器中数据定义只能为全局变量。*/
// 步进电机
code uchar tr[] = {0x11, 0x33, 0x22, 0x66, 0x44, 0xcc, 0x88, 0x99};
code uchar tl[] = {0x11, 0x99, 0x88, 0xcc, 0x44, 0x66, 0x22, 0x33};
code uchar gogo[] = {0x11, 0x93, 0x82, 0xc6, 0x44, 0x6c, 0x28, 0x39};
code uchar again[] = {0x11, 0x39, 0x28, 0x6c, 0x44, 0xc6, 0x82, 0x93};
// 数码管
code uchar tab[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e};

// XDATA的申明把变量放到内部扩展RAM中
xdata uchar mapp[8][8]; // 地图数据

xdata uchar hight[8][8] = {0}; // 等高表数据
code uchar x[4] = {0, 1, 0, -1};
code uchar y[4] = {1, 0, -1, 0};
// P4口声明
sfr P4 = 0xe8;
// 数码管接口定义
sbit t1 = P4 ^ 2;
sbit t2 = P4 ^ 3;
// 蜂鸣器接口定义
sbit beep = P3 ^ 7;
// 小车坐标
uchar cx = 0, cy = 0;
// 小车的绝对方向
uchar absD = 0;
// 红外地址接口定义
sbit A0 = P4 ^ 0;
sbit A1 = P2 ^ 0;
sbit A2 = P2 ^ 7;
// 红外传感器接收信号口定义（接收到信号值为 0）
sbit irR1 = P2 ^ 1;                                // 前
sbit irR2 = P2 ^ 2;                                // 左外
sbit irR3 = P2 ^ 3;                                // 左
sbit irR4 = P2 ^ 4;                                // 右
sbit irR5 = P2 ^ 5;                                // 右外
bit irC = 0, irL = 0, irR = 0, irLU = 0, irRU = 0; // 定义红外传感器检测状态全局位变量，为 0 无障碍
uchar i, j;
// 红外发射控制宏定义（传入传感器组号）
void ir_on(uchar num)
{
    A0 = (num)&0x01;
    A1 = (num)&0x02;
    A2 = (num)&0x04;
}
// T2中断初始化函数，总共只执行一次，需要在此启动定时器2
// 在12M晶振下设定T2中断时间us数，us范围为0-65535
void init_t2(uint us)
{ // 定时器初始化
    // T2中断允许
    EA = 1;
    ET2 = 1;
    // T2CON默认配置16位自动重载定时器
    // T2MOD默认配置递增计数
    TH2 = RCAP2H = (65536 - us) / 256;
    TL2 = RCAP2L = (65536 - us) % 256; // 设定T2自动重装载寄存器和计数器初值
    TR2 = 1;                           // 启动定时器2
}

// 定时器中断服务函数
void tim2() interrupt 5
{
    static bit ir = 0; // 标志本次中断时发射还是接收
    static uchar n = 1;
    TF2 = 0; // 清除T2中断标志位
    if (!ir) // 设置发光二极管发射红外光
        ir_on(n - 1);
    else // 检测各个方向接收管返回的电平
    {
        if (n == 1)
        {
            if (irR1)    // 无墙
                irC = 0; // 清零标志变量
            else
                irC = 1;
        } // 置位标志变量
        else if (n == 2)
        {
            if (irR2)irLU = 0;
            else irLU = 1;
        }
        else if (n == 3)
        {
            if (irR3)irL = 0;
            else irL = 1;
}
        else if (n == 4)
        {
            if (irR4)irR = 0;
            else irR = 1;
        }
        else if (n == 5)
        {
            if (irR5)irRU = 0;
            else irRU = 1;
        }
        n++;
    }
    if (n > 5)n = 1;
    ir = ~ir; // 改变接收、发射状态
}
// 初始化迷宫地图
void init_map(uchar mapp[8][8])
{
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            mapp[i][j] = 0xff;
}
// 延时函数
void delay_ms(uint z)
{
    while (--z)
    {
        _nop_();
        i = 2,j = 199;
        do
        {
            while (--j);
        } while (--i);
    }
}
// 数码管显示
void display(uchar n1, uchar n2)
{
    t1 = 1,t2 = 0,P0 = tab[n1];
    t1 = 0,t2 = 1,P0 = tab[n2];
}

typedef struct
{
    uchar x, y;
} xy;

// 记录迷宫信息
void rec()
{
    if (mapp[cx][cy] == 0xff) // 如果当前的格子未被探测过
    {//依次检测正前方、右方、左方
        uchar wall = 0x10 << ((absD + 2) % 4); // |= 意思为：按位或后赋值,左移运算符（<<）将一个运算对象的各二进制位全部左移若干位
        if (irC == 1)wall |= 0x01 << absD;
        if (irR == 1)wall |= 0x01 << ((absD + 1) % 4);
        if (irL == 1)wall |= 0x01 << ((absD + 3) % 4);
        mapp[cx][cy] = mapp[cx][cy] & wall;
    }
}
// 特判回路
void special()
{
    if (irLU && irL && !irRU && !irR && irC) // 启动左电机
        for (j = 0; j < 21; j++)
            for (i = 0; i < 8; i++)
                P1 = tr[i],delay_ms(3);
    if (irRU && irR && !irLU && !irL && irC) // 启动左电机
        for (j = 0; j < 21; j++)
            for (i = 0; i < 8; i++)
                P1 = tl[i], delay_ms(3);
}
// 左修正
void tllrepair()
{
    for (j = 0; j < 1; j++)
        for (i = 0; i < 8; i++)
            P1 = tl[i],delay_ms(3);
}
// 右修正
void trrrepair()
{
    uchar i, j;
    for (j = 0; j < 1; j++)
        for (i = 0; i < 8; i++)
            P1 = tr[i],delay_ms(3);
}
// 修正
void repair()
{
    if (!irC)
        // 如果没有检测到前方但是检测到了右方和右前方或者左方和左前方
        while ((irRU == 1 && irR == 1) || (irLU == 1 && irL == 1))
        {// 如果右前方被检测到
            if (irRU == 1)tllrepair();
            // 如果左前方被检测到
            if (irLU == 1)trrrepair();
        }
}

// 旋转角度
void turn(uchar opp)
{
    uchar n;
    if (opp != 0)
    {
        // 相对方向为右方或后方
        if (opp == 1 || opp == 3)n = 50;
        // 相对方向为左方
        if (opp == 2)n = 101;
        for (j = 0; j < n; j++)
            for (i = 0; i < 8; i++)
            {
                if (opp == 1 || opp == 2)P1 = tr[i];
                if (opp == 3)P1 = tl[i];
                delay_ms(3);
            }
        absD = (absD + opp) % 4; // 取模
    }
    delay_ms(500);
}

// 直线前行
void run()
{
    for (i = 0; i < 104; i++)   // 转108圈走一步
        for (j = 0; j < 8; j++) // 电机的八拍
        {
            repair(); // 修正
           special(),P1=gogo[j],delay_ms(4)，repair(); 
            special();
        }
    delay_ms(100);
    if (absD == 0) // 如果此时小车车头方向为前方
        cy ++;   // 小车的y坐标加1，下同
    else if (absD == 1)
        cx ++;
    else if (absD == 2)
        cy --;
    else if (absD == 3)
        cx --;
}
// 判断此方向是否为新格子
uchar isnew(uchar opp)
{
    if (cx > 0 || cy > 0)
        return mapp[cx + x[opp]][cy + y[opp]] == 0xff;
    return 0;
}
// 相对方向->绝对方向
uchar rel_to_abs(uchar opp)
{
    return (absD + opp) % 4;
}
// 右手法则
uchar search(uchar mapp[8][8])
{
    // 检测
    if (irR == 0 && isnew(rel_to_abs(1))) // 右边没走过
        beep = 0, delay_ms(100), beep = 1,return 1;
    if (irC == 0 && isnew(rel_to_abs(0))) // 再检测前边
        beep = 0, delay_ms(100), beep = 1,return 0;
    if (irL == 0 && isnew(rel_to_abs(3))) // 再检测左边
        beep = 0, delay_ms(100), beep = 1,return 3;
    // 到此即可判断周围已经没有新路可走，就执行回溯走来时的路
    if (mapp[cx][cy] / 16 == 1)return (4 + 0 - absD) % 4; // abs_to_rel(0);
    if (mapp[cx][cy] / 16 == 2)return (4 + 1 - absD) % 4; // abs_to_rel(1);
    if (mapp[cx][cy] / 16 == 4)return (4 + 2 - absD) % 4; // abs_to_rel(2);
    if (mapp[cx][cy] / 16 == 8)return (4 + 3 - absD) % 4; // abs_to_rel(3);
    return 2;
}
// 反向查找最短路
void path()
{
    uchar x = 7, y = 7;
    while (x != 0 || y != 0) // 如果还没到起点
    {
        // 扫描四个方向
        if ((hight[x][y] - 1 == hight[x + 1][y]) && !((mapp[x][y] >> 1) & 0x01))
            x++,mapp[x][y] = (mapp[x][y] | 0xf0) & 0x8f;
        else if (hight[x][y] - 1 == hight[x - 1][y] && !((mapp[x][y] >> 3) & 0x01))
            x--,mapp[x][y] = (mapp[x][y] | 0xf0) & 0x2f;
        else if (hight[x][y] - 1 == hight[x][y + 1] && !(mapp[x][y] & 0x01))
            y++,mapp[x][y] = (mapp[x][y] | 0xf0) & 0x4f;
        else if (hight[x][y] - 1 == hight[x][y - 1] && !((mapp[x][y] >> 2) & 0x01))
            y--,mapp[x][y] = (mapp[x][y] | 0xf0) & 0x1f;
        display(x, y),delay_ms(150);
    }
}
// 制作等高表
void bfs(begx, begy, endx, endy)
{
    xy q[25],xy head,xy temp;
    uchar lent = 1;  // 初始化队列长度标记量、迭代下标
    uchar i;         // 初始化迭代下标
    init_map(hight); // 初始化等高表
    hight[begy][begx] = 0;
    q[0].x = begx,q[0].y = begy;
    while (lent != 0) // 当对列不为空
    {
        display(temp.x, temp.y);
        delay_ms(200);
        head = q[0]; // 队首元素出列
        lent--;
        for (i = 0; i < lent; i++)
            q[i] = q[i + 1];
        for (i = 0; i < 4; i++) // 判断四个方向
        {
            temp = head; // 保留队首元素
            if (i == 0)temp.y = head.y - 1;
            if (i == 1)temp.x = head.x + 1;
            if (i == 2)temp.y = head.y + 1;
            if (i == 3)temp.x = head.x - 1;
            if (temp.x > 127 || temp.y > 127)continue;
            if (!((mapp[temp.x][temp.y] >> i) & 0x01) && hight[temp.x][temp.y] == 0xff) // 如果该坐标连通且第一次访问
            {
                hight[temp.x][temp.y] = hight[head.x][head.y] + 1; // 该坐标等高表+1
                q[lent++] = temp;                                  // 该坐标入队
            }
        }
    }
}
void init()
{
 uchar a; // 用于记录旋转方向
    init_map(mapp), init_map(hight), delay_ms(1000);
    mapp[0][0] = 0x1e, hight[0][0] = 0;
    rec(), run(); // 前行
    while (1)
    {
        delay_ms(100), rec(); // 记录迷宫信息
        a = search(mapp), turn(a);
        run(), display(cx, cy);
        if ((!cx && !cy) || (cx == 7 && cy == 7)) // 判断是否到达终点或起点
            beep = 0, delay_ms(1000), beep = 1;
        if (!cx && !cy) // 回到起点，开始冲刺
        {
            bfs(0, 0, 7, 7); // 广搜制作等高表
            path();          // 反向查找最短路冲刺
        }
    }
}
void qrun()
{
    for (j = 0; j < 107; j++)   // 步长，转107圈走一步
        for (i = 0; i < 8; i++) // 电机的八拍
            repair(), P1 = gogo[i], delay_ms(4), repair();
}
void turnleft()
{
    for (j = 0; j < 51; j++)
        for (i = 0; i < 8; i++)
            P1 = tl[i], delay_ms(3);
    delay_ms(100);
}
void turnback()
{
    for (j = 0; j < 102; j++)
        for (i = 0; i < 8; i++)
            P1 = tr[i], delay_ms(3);
    delay_ms(100);
}
void turnright()
{
    for (j = 0; j < 51; j++)
        for (i = 0; i < 8; i++)
            P1 = tr[i], delay_ms(3);
    delay_ms(100);
}
// 测试某个方向的红外
void ir_test()
{
    while (1)
    {
        if (irL)
            t1 = 1, t2 = 0, P0 = tab[3];
        if (irR)
            t1 = 1, t2 = 0, P0 = tab[4];
        if (irLU)
            t1 = 1, t2 = 0, P0 = tab[2];
        if (irRU)
            t1 = 1, t2 = 0, P0 = tab[5];
        if (irC)
            t1 = 1, t2 = 0, P0 = tab[1];
    }
}

void main()
{
    init_t2(2500);
    // ir_test();
    init();
}