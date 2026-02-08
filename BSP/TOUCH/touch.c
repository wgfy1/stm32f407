#include "touch.h" 
#include "lcd.h"
#include "delay.h"
#include "stdlib.h"
#include "math.h"
#include "usart.h"
#include "stdio.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

//////////////////////////////////////////////////////////////////////////////////	 
//支持电阻触摸屏芯片：ADS7843/7846/UH7843/7846/XPT2046/TSC2046等	   
//STM32F4开发板-函数库版本
//淘宝店铺：http://mcudev.taobao.com								   
//********************************************************************************
//修改说明
//V1.1 20140721
//修复MDK在-O2优化时,触摸屏无法读取的bug.在TP_Write_Byte函数里面增加一个延时,即可解决.
//修改说明
//V1.2 20231115
//将校准参数保存到FLASH中，替代原来的EEPROM存储
//////////////////////////////////////////////////////////////////////////////////

// FLASH存储参数配置
#define TP_PARAM_FLASH_SECTOR    FLASH_SECTOR_11  // 使用扇区11存储参数
#define TP_PARAM_FLASH_START     0x080E0000       // 扇区11的起始地址（STM32F407VET6）
#define TP_PARAM_MAGIC           0x55AA55AA       // 魔术字，用于验证参数的有效性

// 校准参数结构体
typedef struct {
    uint32_t magic;      // 魔术字
    float xfac;          // X方向校准因子
    float yfac;          // Y方向校准因子
    int16_t xoff;        // X方向偏移量
    int16_t yoff;        // Y方向偏移量
    uint8_t touchtype;   // 触摸类型
} tp_param_t;

_m_tp_dev tp_dev=
{
	TP_Init,
	TP_Scan,
	TP_Adjust,
	0,
	0, 
	0,
	0,
	0,
	0,			
	0,
	0,			
};					
//默认touchtype=0,竖屏.
uint8_t CMD_RDX=0XD0;
uint8_t CMD_RDY=0X90;
 	 			    					   
//SPI写数据
//向触摸屏IC写入1byte数据    
//num:要写入的数据
void TP_Write_Byte(uint8_t num)    
{  
	uint8_t count=0;   
	for(count=0;count<8;count++)  
	{ 	  
		if(num&0x80)T_MOSI(1);  // 修改：TDIN -> T_MOSI
		else T_MOSI(0);   
		num<<=1;    
		T_CLK(0);  // 修改：TCLK -> T_CLK
		delay_us(1);
		T_CLK(1);		//上升沿有效	        
	}		 			    
} 		 
//SPI读数据 
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读取到的数据	    
uint16_t TP_Read_AD(uint8_t CMD)	
{ 	 
	uint8_t count=0; 	 
	uint16_t Num=0; 
	T_CLK(0);		//先拉低时钟 	 
	T_MOSI(0); 	//拉低数据线
	T_CS(0); 		//选中触摸屏IC
	TP_Write_Byte(CMD);//发送命令字
	delay_us(6);//ADS7846的转换时间最长为6us
	T_CLK(0); 	     	 
	delay_us(1);    	    
	T_CLK(1);		//给1个时钟，清除BUSY
	delay_us(1);    
	T_CLK(0); 	     	 
	for(count=0;count<16;count++)//读出16位数据,只有高12位有效 
	{ 				 
		Num<<=1; 	 
		T_CLK(0);	//下降沿有效  	    	    
		delay_us(1);    
 		T_CLK(1);
 		if(T_MISO)Num++;	 
	}  	
	Num>>=4;   	//只有高12位有效.
	T_CS(1);		//释放片选	 
	
	
	
	return(Num);   
}
//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读取到的数据
#define READ_TIMES 8 	//读取次数
#define LOST_VAL 1	  	//丢弃值
static uint16_t TP_Read_XOY(uint8_t xy)
{
	uint16_t i, j;
	uint16_t buf[READ_TIMES];
	uint16_t sum=0;
	uint16_t temp;
	for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);		 		    
	for(i=0;i<READ_TIMES-1; i++)//排序
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//升序排列
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 
//读取x,y坐标
//最小值不能小于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功;
static uint8_t TP_Read_XY(uint16_t *x, uint16_t *y)
{
    uint16_t xtemp, ytemp;
    uint8_t retry = 5;  // 增加重试次数
    
    // 尝试多次读取，确保稳定
    while (retry--) {
        xtemp = TP_Read_XOY(CMD_RDX);
        ytemp = TP_Read_XOY(CMD_RDY);
        
        // 放宽坐标范围检查，避免快速点击时误判
        if (xtemp >= 30 && xtemp <= 3970 && ytemp >= 30 && ytemp <= 3970) {
            *x = xtemp;
            *y = ytemp;
            return 1; // 读数成功
        }
        
        // 短暂延时，避免连续读取干扰
        delay_us(10);
    }
    
    // 多次读取失败，返回上次有效值而不是0
    // 避免快速点击时坐标突然变为0
    return 0; // 读数失败
}
//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.	   
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功;
#define ERR_RANGE 50 //误差范围 
static uint8_t TP_Read_XY2(uint16_t *x, uint16_t *y) 
{
    uint16_t x1, y1;
    uint16_t x2, y2;
    uint8_t flag;
    
    flag = TP_Read_XY(&x1, &y1);   
    if (flag == 0) return 0;
    
    flag = TP_Read_XY(&x2, &y2);   
    if (flag == 0) return 0;
    
    // X轴偏差检查
    if (!(((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + ERR_RANGE))))
        return 0;
    
    // Y轴更严格的偏差检查（Y轴可能更容易受到噪声影响）
    if (!(((y2 <= y1 && y1 < y2 + ERR_RANGE/2) || (y1 <= y2 && y2 < y1 + ERR_RANGE/2))))
        return 0;
    
    *x = (x1 + x2) / 2;
    *y = (y1 + y2) / 2;
    return 1;
}
//////////////////////////////////////////////////////////////////////////////////		  
//与LCD有关的函数  
//画一个触摸点
//用于校准
//x,y:坐标
//color:颜色
void TP_Drow_Touch_Point(uint16_t x,uint16_t y,uint16_t color)
{
	POINT_COLOR=color;
	LCD_DrawLine(x-12,y,x+13,y);//横线
	LCD_DrawLine(x,y-12,x,y+13);//竖线
	LCD_DrawPoint(x+1,y+1);
	LCD_DrawPoint(x-1,y+1);
	LCD_DrawPoint(x+1,y-1);
	LCD_DrawPoint(x-1,y-1);
	LCD_Draw_Circle(x,y,6);//画圆圈
}	  
//画一个大点(2 * 2的点)		   
//x,y:坐标
//color:颜色
void TP_Draw_Big_Point(uint16_t x,uint16_t y,uint16_t color)
{	    
	POINT_COLOR=color;
	LCD_DrawPoint(x,y);//中心点 
	LCD_DrawPoint(x+1,y);
	LCD_DrawPoint(x,y+1);
	LCD_DrawPoint(x+1,y+1);	 	  	
}						  
//////////////////////////////////////////////////////////////////////////////////		  
//触摸屏扫描
//tp:0,屏幕坐标;1,物理坐标(校准等场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
//触摸屏扫描
//tp:0,屏幕坐标;1,物理坐标(校准等场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
uint8_t TP_Scan(uint8_t tp)
{
    if (HAL_GPIO_ReadPin(T_PEN_GPIO_Port, T_PEN_Pin) == 0) { // 有按键按下
        if (tp) {
            // 读取物理坐标
            if (TP_Read_XY2(&tp_dev.x[0], &tp_dev.y[0])) {
                // 物理坐标异常检查
                if (tp_dev.x[0] > 4000) tp_dev.x[0] = 0;
                if (tp_dev.y[0] > 4000) tp_dev.y[0] = 0;
            }
        } else {
            // 读取物理坐标并转换为屏幕坐标
            uint16_t phys_x, phys_y;
            static uint16_t last_valid_x = 0, last_valid_y = 0; // 保存上次有效坐标
            
            if (TP_Read_XY2(&phys_x, &phys_y)) {
                // 放宽物理坐标有效性检查范围
                if (phys_x >= 30 && phys_x <= 3970 && phys_y >= 30 && phys_y <= 3970) {
                    // 执行坐标转换
                    int32_t temp_x = (int32_t)(tp_dev.xfac * phys_x + tp_dev.xoff);
                    int32_t temp_y = (int32_t)(tp_dev.yfac * phys_y + tp_dev.yoff);
                    
                    // 确保坐标为非负值
                    if (temp_x < 0) temp_x = 0;
                    if (temp_y < 0) temp_y = 0;
                    
                    // 确保坐标在屏幕范围内
                    if ((uint32_t)temp_x >= makerbase_lcd.width) temp_x = makerbase_lcd.width - 1;
                    if ((uint32_t)temp_y >= makerbase_lcd.height) temp_y = makerbase_lcd.height - 1;
                    
                    tp_dev.x[0] = (uint16_t)temp_x;
                    tp_dev.y[0] = (uint16_t)temp_y;
                    
                    // 保存有效坐标
                    last_valid_x = tp_dev.x[0];
                    last_valid_y = tp_dev.y[0];
                } else {
                    // 物理坐标异常，使用上次有效坐标而不是0
                    tp_dev.x[0] = last_valid_x;
                    tp_dev.y[0] = last_valid_y;
                }
            } else {
                // 读取失败，使用上次有效坐标而不是0
                tp_dev.x[0] = last_valid_x;
                tp_dev.y[0] = last_valid_y;
            }
        }
        
        if ((tp_dev.sta & TP_PRES_DOWN) == 0) {
            tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;
            tp_dev.x[4] = tp_dev.x[0];
            tp_dev.y[4] = tp_dev.y[0];
        }
    } else {
        if (tp_dev.sta & TP_PRES_DOWN) {
            tp_dev.sta &= ~(1 << 7);
        } else {
            tp_dev.x[4] = 0;
            tp_dev.y[4] = 0;
            tp_dev.x[0] = 0xffff;
            tp_dev.y[0] = 0xffff;
        }
    }
    return tp_dev.sta & TP_PRES_DOWN;
}
//////////////////////////////////////////////////////////////////////////	 
//保存校准参数到FLASH
void TP_Save_Adjdata(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    tp_param_t param;
    uint32_t* pdata;
    
    // 准备参数
    param.magic = TP_PARAM_MAGIC;
    param.xfac = tp_dev.xfac;
    param.yfac = tp_dev.yfac;
    param.xoff = tp_dev.xoff;
    param.yoff = tp_dev.yoff;
    param.touchtype = tp_dev.touchtype;
    
    // 解锁FLASH
    HAL_FLASH_Unlock();
    
    // 擦除扇区
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector = TP_PARAM_FLASH_SECTOR;
    EraseInitStruct.NbSectors = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        // 擦除失败，处理错误
        HAL_FLASH_Lock();
        return;
    }
    
    // 写入参数
    pdata = (uint32_t*)&param;
    for (int i = 0; i < sizeof(tp_param_t) / 4; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, TP_PARAM_FLASH_START + i * 4, pdata[i]) != HAL_OK)
        {
            // 写入失败，处理错误
            HAL_FLASH_Lock();
            return;
        }
    }
    
    // 锁定FLASH
    HAL_FLASH_Lock();
}	 
//从FLASH读取校准参数
//返回值：1，成功读取参数
//        0，读取失败，需要重新校准
uint8_t TP_Get_Adjdata(void)
{
    tp_param_t* param = (tp_param_t*)TP_PARAM_FLASH_START;
    
    // 检查魔术字
    if (param->magic != TP_PARAM_MAGIC)
    {
        return 0;
    }
    
    // 读取参数
    tp_dev.xfac = param->xfac;
    tp_dev.yfac = param->yfac;
    tp_dev.xoff = param->xoff;
    tp_dev.yoff = param->yoff;
    tp_dev.touchtype = param->touchtype;
    
    // 设置指令
    if (tp_dev.touchtype)
    {
        CMD_RDX = 0X90;
        CMD_RDY = 0XD0;
    }
    else
    {
        CMD_RDX = 0XD0;
        CMD_RDY = 0X90;
    }
    
    return 1;
}	 
//显示校准提示字符串
//uint8_t* const TP_REMIND_MSG_TBL="Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";
 					  
//显示校准信息(中间变量)
void TP_Adj_Info_Show(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t x3,uint16_t y3,uint16_t fac)
{	  
	POINT_COLOR=RED;
	LCD_ShowString(40,160,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"x1:");
 	LCD_ShowString(40+80,160,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"y1:");
 	LCD_ShowString(40,180,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"x2:");
 	LCD_ShowString(40+80,180,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"y2:");
	LCD_ShowString(40,200,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"x3:");
 	LCD_ShowString(40+80,200,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"y3:");
	LCD_ShowString(40,220,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"x4:");
 	LCD_ShowString(40+80,220,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"y4:");  
 	LCD_ShowString(40,240,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"fac is:");     
	LCD_ShowNum(40+24,160,x0,4,16);		//显示数值
	LCD_ShowNum(40+24+80,160,y0,4,16);	//显示数值
	LCD_ShowNum(40+24,180,x1,4,16);		//显示数值
	LCD_ShowNum(40+24+80,180,y1,4,16);	//显示数值
	LCD_ShowNum(40+24,200,x2,4,16);		//显示数值
	LCD_ShowNum(40+24+80,200,y2,4,16);	//显示数值
	LCD_ShowNum(40+24,220,x3,4,16);		//显示数值
	LCD_ShowNum(40+24+80,220,y3,4,16);	//显示数值
 	LCD_ShowNum(40+56,240,fac,3,16); 	//显示数值,正常值在95~105之间.
}
		 
//触摸屏校准代码
//得到四个校准点
void TP_Adjust(void)
{								 
	uint16_t pos_temp[4][2];//坐标缓存值
	uint8_t  cnt=0;	
	uint16_t d1,d2;
	uint32_t tem1,tem2;
	double fac; 	
	uint16_t outtime=0;//超时时间
 	cnt=0;				
	POINT_COLOR=BLUE;
	BACK_COLOR =WHITE;
	LCD_Clear(WHITE);//清屏   
	POINT_COLOR=RED;//红色 
	LCD_Clear(WHITE);//清屏 	   
	POINT_COLOR=BLACK;
	//LCD_ShowString(40,40,160,100,16,(uint8_t*)TP_REMIND_MSG_TBL);//显示提示信息
	TP_Drow_Touch_Point(20,20,RED);//画点1 
	tp_dev.sta=0;//清除触发信号 
	tp_dev.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误	 
	while(1)//如果连续10秒钟没有按下,则自动退出
	{
		tp_dev.scan(1);//扫描物理坐标
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)//按键按下了一次(此时按键松开了.)
		{	
			outtime=0;		
			tp_dev.sta&=~(1<<6);//标记按键已经被处理过了.
						   			   
			pos_temp[cnt][0]=tp_dev.x[0];
			pos_temp[cnt][1]=tp_dev.y[0];
			cnt++;	  
			switch(cnt)
			{			   
				case 1:						 
					TP_Drow_Touch_Point(20,20,WHITE);				//清除点1 
					TP_Drow_Touch_Point(makerbase_lcd.width-20,20,RED);	//画点2
					break;
				case 2:
 					TP_Drow_Touch_Point(makerbase_lcd.width-20,20,WHITE);	//清除点2
					TP_Drow_Touch_Point(20,makerbase_lcd.height-20,RED);	//画点3
					break;
				case 3:
 					TP_Drow_Touch_Point(20,makerbase_lcd.height-20,WHITE);			//清除点3
 					TP_Drow_Touch_Point(makerbase_lcd.width-20,makerbase_lcd.height-20,RED);	//画点4
					break;
				case 4:	 //全部四个点已经得到
	    		    //对边相等
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,2的距离
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到3,4的距离
					fac=(float)d1/d2;
					if(fac<0.85||fac>1.15||d1==0||d2==0)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(makerbase_lcd.width-20,makerbase_lcd.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,3的距离
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,4的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(makerbase_lcd.width-20,makerbase_lcd.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						continue;
					}//正确
								   
					//对角线
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,4的距离
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,3的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(makerbase_lcd.width-20,makerbase_lcd.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						continue;
					}//正确
					//计算校准因子
					tp_dev.xfac=(float)(makerbase_lcd.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac		 
					tp_dev.xoff=(makerbase_lcd.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff
						  
					tp_dev.yfac=(float)(makerbase_lcd.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
					tp_dev.yoff=(makerbase_lcd.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff  
					if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//触屏和预设的相反了.
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(makerbase_lcd.width-20,makerbase_lcd.height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(20,20,RED);								//画点1
						LCD_ShowString(40,26,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"TP Need readjust!");
						tp_dev.touchtype=!tp_dev.touchtype;//修改触屏类型.
						if(tp_dev.touchtype)//X,Y坐标与屏幕相反
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;	 
						}else				   //X,Y坐标与屏幕相同
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;	 
						}			    
						continue;
					}		
					POINT_COLOR=BLUE;
					LCD_Clear(WHITE);//清屏
					LCD_ShowString(35,110,makerbase_lcd.width,makerbase_lcd.height,16,(uint8_t *)"Touch Screen Adjust OK!");//鏍″噯鎴愬姛OK!");//校准成功
				delay_ms(1000);

				
				// 保存校准参数到FLASH
				TP_Save_Adjdata();

				delay_ms(3000);  // 显示3秒
				LCD_Clear(WHITE);//清屏   
				return;//校准完成
 	}
}	
			

	} 
}
//触摸屏初始化		    
//返回值:0,没有进行校准
//       1,已经校准
uint8_t TP_Init(void)
{
    // 先尝试从FLASH读取校准参数
    if (TP_Get_Adjdata())
    {
        return 1;
    }
    
    // 如果读取失败，进行校准
    LCD_Clear(WHITE);
    TP_Adjust();
    
    return 1;
}