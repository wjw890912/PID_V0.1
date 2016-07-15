

 
/**




**/
#include <math.h>
#include <string.h>
#include "stm32f10x.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "pidctrl.h"
#include "ds18b20.h" 
#include "main.h"
#ifndef ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#endif

#define WINDUP_ON(_pid)         (_pid->features & PID_ENABLE_WINDUP)
#define DEBUG_ON(_pid)          (_pid->features & PID_DEBUG)
#define SAT_MIN_ON(_pid)        (_pid->features & PID_OUTPUT_SAT_MIN)
#define SAT_MAX_ON(_pid)        (_pid->features & PID_OUTPUT_SAT_MAX)
#define HIST_ON(_pid)           (_pid->features & PID_INPUT_HIST)


#define PID_CREATTRM_INTERVAL  	(250*4)  /*250*2 ms ---  pid once that again and again*/

#ifdef USING_PID





/*****external SSR ctrl module*********/
 
 uint8_t Interrupt_Extern=0;
 uint16_t Adj_Power_Time=0;
 uint16_t Power_Adj=0;


 void initBta16TMER()
 {
 //ʹ����һ��100US�Ķ�ʱ����
 //��ʼ��
 uint16_t 	usPrescalerValue;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	//====================================ʱ�ӳ�ʼ��===========================
	//ʹ�ܶ�ʱ��2ʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	//====================================��ʱ����ʼ��===========================
	//��ʱ��ʱ�������˵��
	//HCLKΪ72MHz��APB1����2��ƵΪ36MHz
	//TIM2��ʱ�ӱ�Ƶ��Ϊ72MHz��Ӳ���Զ���Ƶ,�ﵽ���
	//TIM2�ķ�Ƶϵ��Ϊ3599��ʱ���Ƶ��Ϊ72 / (1 + Prescaler) = 100KHz,��׼Ϊ10us
	//TIM������ֵΪusTim1Timerout50u	
	usPrescalerValue = (uint16_t) (72000000 / 25000) - 1;
	//Ԥװ��ʹ��
	TIM_ARRPreloadConfig(TIM4, ENABLE);
	//====================================�жϳ�ʼ��===========================
	//����NVIC���ȼ�����ΪGroup2��0-3��ռʽ���ȼ���0-2����Ӧʽ���ȼ�
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//�������жϱ�־λ
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	//��ʱ��2����жϹر�
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
	//��ʱ��3����
	TIM_Cmd(TIM4, DISABLE);

	TIM_TimeBaseStructure.TIM_Prescaler = usPrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = (uint16_t)( 1);//100us
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_SetCounter(TIM4, 0);
	TIM_Cmd(TIM4, DISABLE);
 
 }
 void TIM4_IRQHandler(void)
{
	
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		
		TIM_ClearFlag(TIM4, TIM_FLAG_Update);	     //���жϱ��
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);	 //�����ʱ��TIM3����жϱ�־λ

	    Adj_Power_Time++;//increase time 
  	   AutoRunPowerAdjTask();
	}
	
}	
void EXTI4_IRQHandler(void)
{
     
	 	Interrupt_Extern=1;
		Adj_Power_Time=0;
    /* Clear the Key Button EXTI line pending bit */
    EXTI_ClearITPendingBit(EXTI_Line4);

   
}
void InitzeroGPIO()
 {
 
//DrvGPIO_SetIntCallback(pfP0P1Callback, pfP2P3P4Callback);
//DrvGPIO_EnableInt(E_PORT2, E_PIN7, E_IO_RISING, E_MODE_EDGE);
	   //��������Ϊ�ⲿ�ж��½�����Ч

	 NVIC_InitTypeDef NVIC_InitStructure;
	 GPIO_InitTypeDef GPIO_InitStructure;
     EXTI_InitTypeDef EXTI_InitStructure;

	  /* Enable GPIOC and GPIOB clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
  
  /* Enable AFIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

 	/* Enable the EXTI3 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

	/* configure PB4 as SSR ctrl pin */

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	  GPIO_SetBits(GPIOE,GPIO_Pin_2);
	  GPIO_ResetBits(GPIOE,GPIO_Pin_2);
	  	/* configure PB3 as external interrupt */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

     GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
	 
	    /* Configure  EXTI Line to generate an interrupt on falling edge */
    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

	/* Clear the Key Button EXTI line pending bit */
	EXTI_ClearITPendingBit(EXTI_Line4); 
	  
 }


/*
���ù��ʰٷֱ�

 Power ��Χ��0-100%
 0%����С����
 100%����߹���
*/
void Set_Power(char Power)
{

	//	Power_Adj=0	�����������
	//	Power_Adj=100�

	 Power_Adj=100-Power;



}

void Trdelay()
{

	  uint32_t i;
	  for(i=500;i;i--);


}
 void Trigger_SSR_Task()
 {

	 
   #if SSR_ULN2004

	GPIO_ResetBits(GPIOE,GPIO_Pin_2);	

	Trdelay();

	GPIO_SetBits(GPIOE,GPIO_Pin_2);

	#else

	GPIO_ResetBits(GPIOE,GPIO_Pin_2);	

	Trdelay();

	GPIO_SetBits(GPIOE,GPIO_Pin_2);


	#endif
	 
				   
 }

 /*----------Power Task Auto  Run---------------------------------------------------------------------*/
 void AutoRunPowerAdjTask()
 {
		   if(Interrupt_Extern==1)
		 {	
				 if( Adj_Power_Time>=Power_Adj)
				   {
				   
				   	Trigger_SSR_Task();		 
		 		   				 
				   	 
					 Interrupt_Extern=0;

				   }
		 		  	
			
		 }	


 }


 void InitSSR(void)
 {
 	InitzeroGPIO();
 
   initBta16TMER();
 }

 /*start of PID*/

 extern float Temp_pid[1];
 uint32_t PidCreatTrm=0;

 	PID_t pid;
	float result,kj;
	



extern void DS1820main(void); 		
//PID

void init_PID(void)
{
	/*��ʼ��PID�ṹ*/
		/*
	  	 ��������=1��
		 ��������=0��
		 ΢������=0��

	   */	
   	 pid_init(&pid, 1, 0, 0);
	  /*ʹ�ܻ��߽���PID����Ĺ���*/	
   //pid_enable_feature(&pid, , 0)		
	/*Ŀ��ֵ�趨*/		 
     pid_set(&pid,40);

}

extern uint8_t Mb2TcpBuff[256];
extern uint32_t Mb2TcpLenth;
extern    char TcpRecvBuf[1500];
extern   uint32_t TcpRecvLenth;


float ProcessTcpSrting(char *buf,char*str)
{
	   char *location=0;
	   uint8_t val[50],i,j,k;
	   float finteger=0,fdecimal=0,fresult=0;

	  location=	strstr(buf,str);//�����ƶ��ַ�,�����ַ�λ�á�
	
	   if(*(location+2)=='=') //��λ׼ȷ��yes into   kp=12.54\r\n				 2,i=1;	10^0+10^1
	   {
	   	  for(i=0;i<50;i++)	//��ʼ����
	   	   {
			    val[i] = *(location+2+i);//��"="��ʼ�����

				if(val[i]=='.')//����Ϊ��������
				{
				   	    k=i-1;//��¼С����Ч���ַ����һ��λ��
						j=0;
						finteger=0;
					while(1)
					{
					
						finteger+=(float)((val[k]-0x30)*pow(10,j));//�����������ֵ�����
						  j++;//ƽ������
						  k--;//��λ����
						 if(val[k]=='=')break;//����ҵ���ʼ��λ����ô�����˴�����
					}


				}

				if(val[i]==',')//�����������ַ�
				{
					   	k=i-1;//��¼С����Ч���ַ����һ��λ��
						j=0;
					  fdecimal=0;
					  while(1)
					  {
						 
						  
					  fdecimal+=(float)((val[k]-0x30)*pow(10,j));//�����������ֵ�����
					  k--;//��λ����
					  j++;//ƽ������
						  if(val[k]=='.')
						  {
						  fdecimal=fdecimal/pow(10,j);	//ת��ΪС��
						  fresult=fdecimal+finteger;
					      return  fresult;
						  }
						 
					  
					  }

					//����С�������յó�����ֵ����
				  
				} 

			}
			//���ִ�е��˴�����û�������κβ��������ش��󼴿�
	   		 return -1;
	   }


}

 /*
                Ҫ��������Ϊ����pidset:kp=1.01,ki=3.1,kd=1.5,sp=20.01,�����ű���ӡ� 
				ȫ����С���㣬������С��λ��
 */
void PidThread(uint32_t Time)
{  
    float kp,ki,kd,sp; 
	float temppid;
	static char Timeonce=0;
     			 
if (Time - PidCreatTrm >= PID_CREATTRM_INTERVAL)
  {
		
        PidCreatTrm =  Time;
		if(TcpRecvLenth>0)
		{

		   kp =  ProcessTcpSrting(TcpRecvBuf,"kp");
		   ki =  ProcessTcpSrting(TcpRecvBuf,"ki");
		   kd =  ProcessTcpSrting(TcpRecvBuf,"kd");
		   sp =	 ProcessTcpSrting(TcpRecvBuf,"sp");

		 pid_init(&pid, kp, ki, kd);
		 pid_set(&pid,sp);
		 TcpRecvLenth=0;


			Mb2TcpBuff[0] ='p';
			Mb2TcpBuff[1] ='i';
			Mb2TcpBuff[2] ='d';
			Mb2TcpBuff[3] ='o';
			Mb2TcpBuff[4] ='k';
			Mb2TcpBuff[5] ='\r';
			Mb2TcpBuff[6] ='\n';

			Mb2TcpLenth   =	 7;
			return;
		}

	  

	   DS1820main();//�ɼ���ǰ�¶�  
	    
	temppid = Temp_pid[0]/100;



     result  =  pid_calculate(&pid,temppid, 1) ;


	Mb2TcpBuff[0] = '\t';
	Mb2TcpBuff[1] = (uint8_t)temppid/10 +0x30;//��ǰ�¶�
	Mb2TcpBuff[2] = (uint8_t)temppid%10 +0x30;//��ǰ�¶�
	Mb2TcpBuff[3] = '.';//С����

	temppid=temppid-(((Mb2TcpBuff[1]-0x30)*10)+(Mb2TcpBuff[2]-0x30));
	temppid=temppid*100;//����2λС��



	Mb2TcpBuff[4] = (uint8_t)temppid/10 +0x30;//��ǰ�¶�
	Mb2TcpBuff[5] = (uint8_t)temppid%10 +0x30;//��ǰ�¶�

	Mb2TcpBuff[6] = '\t';


		
			  
			
		   if((result>100))/*0-100����*/
		   {
				 result=100;
		   	  Set_Power(result);
			  	Mb2TcpBuff[7] = (uint8_t) result/100+0x30 ;//��ǰ����ٷֱ�
	            Mb2TcpBuff[8] = (uint8_t) result%100/10+0x30 ;//��ǰ����ٷֱ�
	            Mb2TcpBuff[9] = (uint8_t) result%100%10+0x30 ;//��ǰ����ٷֱ�


				Mb2TcpBuff[10] = '\r';
				Mb2TcpBuff[11] = '\n';
				
				Mb2TcpLenth   =	 12;
		   }
		   else
		   if((result<0))/*0-100����*/
		   {
				 result=0;
		   	  Set_Power(result);
			  	Mb2TcpBuff[7] = (uint8_t) result/100+0x30 ;//��ǰ����ٷֱ�
	            Mb2TcpBuff[8] = (uint8_t) result%100/10+0x30 ;//��ǰ����ٷֱ�
	            Mb2TcpBuff[9] = (uint8_t) result%100%10+0x30 ;//��ǰ����ٷֱ�


				Mb2TcpBuff[10] = '\r';
				Mb2TcpBuff[11] = '\n';
				
				Mb2TcpLenth   =	 12;
		   }
		   else
		   {
		   
		   	 Set_Power(result);
			 	Mb2TcpBuff[7] = (uint8_t) result/100+0x30 ;//��ǰ����ٷֱ�
	            Mb2TcpBuff[8] = (uint8_t) result%100/10+0x30 ;//��ǰ����ٷֱ�
	            Mb2TcpBuff[9] = (uint8_t) result%100%10+0x30 ;//��ǰ����ٷֱ�


				Mb2TcpBuff[10] = '\r';
				Mb2TcpBuff[11] = '\n';
				
				Mb2TcpLenth   =	 12;
		   }

		if(Timeonce==0)
		{
		TIM_Cmd(TIM4, ENABLE);//just run once
		Timeonce=1;
		}

  }
	
	
}


void pid_enable_feature(PID_t *pid, unsigned int feature, float value)
{
    pid->features |= feature;

    switch (feature) {
        case PID_ENABLE_WINDUP:
            /* integral windup is in absolute output units, so scale to input units */
            pid->intmax = ABS(value / pid->ki);
            break;
        case PID_DEBUG:
            break;
        case PID_OUTPUT_SAT_MIN:
            pid->sat_min = value;
            break;
        case PID_OUTPUT_SAT_MAX:
            pid->sat_max = value;
            break;
        case PID_INPUT_HIST:
            break;
    }
}

/**
 *
 * @param pid
 * @param kp
 * @param ki
 * @param kd
 */
void pid_init(PID_t *pid, float kp, float ki, float kd)
{
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;

	pid->sp = 0;
	pid->error_previous = 0;
	pid->integral = 0;

    pid->features = 0;

    if (DEBUG_ON(pid))
        printf("setpoint,value,P,I,D,error,i_total,int_windup\n");
}

void pid_set(PID_t *pid, float sp)
{
	pid->sp = sp;
	pid->error_previous = 0;
	pid->integral = 0;
}

/**
 *
 * @param pid
 * @param val  	now pv value
 * @param dt   
 * @return
 *			
 *     PID  = [ Kp*e(t)] + [Ki*��(0-t)e(��)d��] + [Kd*de(t)/dt]
 *
�����[ Kp*e(t)]		   ���ͬ�������Ժ���			 --���������Ĳ�ֵ
�����[Ki*��(0-t)e(��)d��]������0-t�ϵ�������ߵĻ���	 --���ڻ��ֵ�����������Դ��������Ĺ�ȥ
΢���[Kd*de(t)/dt]��    ����0-t�ϵ�������ߵ�΢��	 --����΢�ֵ��󵼣�б�ʣ��������Դ���������δ��������.

The PID control scheme is named after its three correcting terms,
whose sum constitutes the manipulated variable (MV). 
The proportional, integral, and derivative terms are summed to calculate the output of the PID controller. 
Defining u(t) as the controller output, the final form of the PID algorithm is:

   	u(t) =MV(t) = [ Kp*e(t)] + [Ki*��(0-t)e(��)d��] + [Kd*de(t)/dt]

where

Kp: Proportional gain, a tuning parameter
Ki: Integral gain, a tuning parameter
Kd: Derivative gain, a tuning parameter
e(t): Error=SP-PV(t)
SP: Set Point
PV(t): Process Variable
t: Time or instantaneous time (the present)
��: Variable of integration; takes on values from time 0 to the present t.
Equivalently, the transfer function in the Laplace Domain of the PID controller is

L(s)=Kp+Ki/s+Kds

where

{ s} s: complex number frequency



Proportional term

Plot of PV vs time, for three values of Kp (Ki and Kd held constant)
The proportional term produces an output value that is proportional to the current error value.
 The proportional response can be adjusted by multiplying the error by a constant Kp, called the proportional gain constant.

The proportional term is given by:

 Pout =Kp*e(t)

A high proportional gain results in a large change in the output for a given change in the error.
If the proportional gain is too high, 
the system can become unstable (see the section on loop tuning). 
In contrast, a small gain results in a small output response to a large input error,
and a less responsive or less sensitive controller. 
If the proportional gain is too low, the control action may be too small when responding to system disturbances. 
Tuning theory and industrial practice indicate that the proportional term should contribute the bulk of the output change.


Steady-state error
Because a non-zero error is required to drive it, 
a proportional controller generally operates with a so-called steady-state error.[a] 
Steady-state error (SSE) is proportional to the process gain and inversely proportional to proportional gain. 
SSE may be mitigated by adding a compensating bias term to the setpoint or output, 
or corrected dynamically by adding an integral term.



Integral term[edit]

Plot of PV vs time, for three values of Ki (Kp and Kd held constant)
The contribution from the integral term is proportional to both the magnitude of the error and the duration of the error. 
The integral in a PID controller is the sum of the instantaneous error over time and gives the accumulated offset that should have been corrected previously. 
The accumulated error is then multiplied by the integral gain ( Ki) and added to the controller output.

The integral term is given by:

Iout =[Ki*��(0-t)e(��)d��]
The integral term accelerates the movement of the process towards setpoint and eliminates the residual steady-state error that occurs with a pure proportional controller. 
However, since the integral term responds to accumulated errors from the past, it can cause the present value to overshoot the setpoint value (see the section on loop tuning).

Derivative term[edit]

Plot of PV vs time, for three values of Kd (Kp and Ki held constant)
The derivative of the process error is calculated by determining the slope of the error over time and multiplying this rate of change by the derivative gain Kd. 
The magnitude of the contribution of the derivative term to the overall control action is termed the derivative gain, Kd.

The derivative term is given by:

Dout = [Kd*de(t)/dt]

Derivative action predicts system behavior and thus improves settling time and stability of the system.[12][13]
 An ideal derivative is not causal, so that implementations of PID controllers include an additional low pass filtering for the derivative term,
  to limit the high frequency gain and noise.[14] Derivative action is seldom used in practice though - by one estimate in only 25% of deployed controllers[14] 
  - because of its variable impact on system stability in real-world applications.[14]

 */
float pid_calculate(PID_t *pid, float pv, float dt)
{
	float i,d, error, total;

	error = pid->sp - pv;
	i = pid->integral + (error * dt);
	d = (error - pid->error_previous) / dt;

    total = (error * pid->kp) + (i * pid->ki) + (d * pid->kd);

    if (DEBUG_ON(pid))
        printf("%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%d\n", 
                pid->sp,pv,
                (error * pid->kp), (i * pid->ki), (d * pid->kd),
                error, pid->integral, ABS(i) == pid->intmax);

    if ( WINDUP_ON(pid) ) {
        if ( i < 0 )
            i = ( i < -pid->intmax ? -pid->intmax : i );
        else
   		    i = ( i < pid->intmax ? i : pid->intmax );
    }
    pid->integral = i;

    if ( SAT_MIN_ON(pid) && (total < pid->sat_min) )
        return pid->sat_min;
    if ( SAT_MAX_ON(pid) && (total > pid->sat_max) )
        return pid->sat_max;

	pid->error_previous = error;
	return total;
}


 /*end of PID*/



 #endif
 
 /*
1���޷��˲������ֳƳ����ж��˲�����
���� A��������
���� ���ݾ����жϣ�ȷ�����β�����������ƫ��ֵ����ΪA��
���� ÿ�μ�⵽��ֵʱ�жϣ�
���� �������ֵ���ϴ�ֵ֮��<=A,�򱾴�ֵ��Ч
���� �������ֵ���ϴ�ֵ֮��>A,�򱾴�ֵ��Ч,��������ֵ,���ϴ�ֵ���汾��ֵ
���� B���ŵ㣺
���� ����Ч�˷���żȻ����������������
���� C��ȱ��
���� �޷��������������Եĸ���
���� ƽ���Ȳ�
 
 
#define A 10
char value;
char filter()
{
   char  new_value;
   new_value = get_ad();
   if ( ( new_value - value > A ) || ( value - new_value > A )
      return value;
   return new_value;
}
 
2����λֵ�˲���
���� A��������
���� ��������N�Σ�Nȡ������
���� ��N�β���ֵ����С����
���� ȡ�м�ֵΪ������Чֵ
���� B���ŵ㣺
���� ����Ч�˷���żȻ��������Ĳ�������
���� ���¶ȡ�Һλ�ı仯�����ı�����������õ��˲�Ч��
���� C��ȱ�㣺
���� ���������ٶȵȿ��ٱ仯�Ĳ�������


#define N  11
char filter()
{
   char value_buf[N];
   char count,i,j,temp;
   for ( count=0;count<N;count++)
   {
      value_buf[count] = get_ad();
      delay();
   }
   for (j=0;j<N-1;j++)
   {
      for (i=0;i<N-j;i++)
      {
         if ( value_buf[i]>value_buf[i+1] )
         {
            temp = value_buf[i];
            value_buf[i] = value_buf[i+1]; 
             value_buf[i+1] = temp;
         }
      }
   }
   return value_buf[(N-1)/2];
}     

3������ƽ���˲���
���� A��������
���� ����ȡN������ֵ��������ƽ������
���� Nֵ�ϴ�ʱ���ź�ƽ���Ƚϸߣ��������Ƚϵ�
���� Nֵ��Сʱ���ź�ƽ���Ƚϵͣ��������Ƚϸ�
���� Nֵ��ѡȡ��һ��������N=12��ѹ����N=4
���� B���ŵ㣺
���� �����ڶ�һ�����������ŵ��źŽ����˲�
���� �����źŵ��ص�����һ��ƽ��ֵ���ź���ĳһ��ֵ��Χ�������²���
���� C��ȱ�㣺
���� ���ڲ����ٶȽ�����Ҫ�����ݼ����ٶȽϿ��ʵʱ���Ʋ�����
���� �Ƚ��˷�RAM

#define N 12
char filter()
{
   int  sum = 0;
   for ( count=0;count<N;count++)
   {
      sum + = get_ad();
      delay();
   }
   return (char)(sum/N);
}

4������ƽ���˲������ֳƻ���ƽ���˲�����
���� A��������
���� ������ȡN������ֵ����һ������
���� ���еĳ��ȹ̶�ΪN
���� ÿ�β�����һ�������ݷ����β,���ӵ�ԭ�����׵�һ������.(�Ƚ��ȳ�ԭ��)
���� �Ѷ����е�N�����ݽ�������ƽ������,�Ϳɻ���µ��˲����
���� Nֵ��ѡȡ��������N=12��ѹ����N=4��Һ�棬N=4~12���¶ȣ�N=1~4
���� B���ŵ㣺
���� �������Ը��������õ��������ã�ƽ���ȸ�
���� �����ڸ�Ƶ�񵴵�ϵͳ
���� C��ȱ�㣺
���� �����ȵ�
���� ��żȻ���ֵ������Ը��ŵ��������ýϲ�
���� �������������������������Ĳ���ֵƫ��
���� ��������������űȽ����صĳ���
���� �Ƚ��˷�RAM
���� 
#define N 12 
char value_buf[N];
char i=0;
char filter()
{
   char count;
   int  sum=0;
   value_buf[i++] = get_ad();
   if ( i == N )   i = 0;
   for ( count=0;count<N,count++)
      sum = value_buf[count];
   return (char)(sum/N);
}

����5����λֵƽ���˲������ֳƷ��������ƽ���˲�����
���� A��������
���� �൱�ڡ���λֵ�˲�����+������ƽ���˲�����
���� ��������N�����ݣ�ȥ��һ�����ֵ��һ����Сֵ
���� Ȼ�����N-2�����ݵ�����ƽ��ֵ
���� Nֵ��ѡȡ��3~14
���� B���ŵ㣺
���� �ں��������˲������ŵ�
���� ����żȻ���ֵ������Ը��ţ������������������������Ĳ���ֵƫ��
���� C��ȱ�㣺
���� �����ٶȽ�����������ƽ���˲���һ��
���� �Ƚ��˷�RAM
���� 
#define N 12
char filter()
{
   char count,i,j;
   char value_buf[N];
   int  sum=0;
   for  (count=0;count<N;count++)
   {
      value_buf[count] = get_ad();
      delay();
   }
   for (j=0;j<N-1;j++)
   {
      for (i=0;i<N-j;i++)
      {
         if ( value_buf[i]>value_buf[i+1] )
         {
            temp = value_buf[i];
            value_buf[i] = value_buf[i+1]; 
             value_buf[i+1] = temp;
         }
      }
   }
   for(count=1;count<N-1;count++)
      sum += value[count];
   return (char)(sum/(N-2));
}

����6���޷�ƽ���˲���
���� A��������
���� �൱�ڡ��޷��˲�����+������ƽ���˲�����
���� ÿ�β��������������Ƚ����޷�����
���� ��������н��е���ƽ���˲�����
���� B���ŵ㣺
���� �ں��������˲������ŵ�
���� ����żȻ���ֵ������Ը��ţ������������������������Ĳ���ֵƫ��
���� C��ȱ�㣺
���� �Ƚ��˷�RAM
����  
�� �ο��ӳ���1��3
����7��һ���ͺ��˲���
���� A��������
���� ȡa=0~1
���� �����˲����=��1-a��*���β���ֵ+a*�ϴ��˲����
���� B���ŵ㣺
���� �������Ը��ž������õ���������
���� �����ڲ���Ƶ�ʽϸߵĳ���
���� C��ȱ�㣺
���� ��λ�ͺ������ȵ�
���� �ͺ�̶�ȡ����aֵ��С
���� ���������˲�Ƶ�ʸ��ڲ���Ƶ�ʵ�1/2�ĸ����ź�
���� 
#define a 50
char value;
char filter()
{
   char  new_value;
   new_value = get_ad();
   return (100-a)*value + a*new_value; 
}

����8����Ȩ����ƽ���˲���
���� A��������
���� �ǶԵ���ƽ���˲����ĸĽ�������ͬʱ�̵����ݼ��Բ�ͬ��Ȩ
���� ͨ���ǣ�Խ�ӽ���ʱ�̵����ݣ�Ȩȡ��Խ��
���� �����²���ֵ��Ȩϵ��Խ����������Խ�ߣ����ź�ƽ����Խ��
���� B���ŵ㣺
���� �������нϴ��ͺ�ʱ�䳣���Ķ���
���� �Ͳ������ڽ϶̵�ϵͳ
���� C��ȱ�㣺
���� ���ڴ��ͺ�ʱ�䳣����С���������ڽϳ����仯�������ź�
���� ����Ѹ�ٷ�Ӧϵͳ��ǰ���ܸ��ŵ����س̶ȣ��˲�Ч����
���� 
#define N 12
char code coe[N] = {1,2,3,4,5,6,7,8,9,10,11,12};
char code sum_coe = 1+2+3+4+5+6+7+8+9+10+11+12;
char filter()
{
   char count;
   char value_buf[N];
   int  sum=0;
   for (count=0,count<N;count++)
   {
      value_buf[count] = get_ad();
      delay();
   }
   for (count=0,count<N;count++)
      sum += value_buf[count]*coe[count];
   return (char)(sum/sum_coe);
}

����9�������˲���
���� A��������
���� ����һ���˲�������
���� ��ÿ�β���ֵ�뵱ǰ��Чֵ�Ƚϣ�
���� �������ֵ����ǰ��Чֵ�������������
���� �������ֵ<>��ǰ��Чֵ���������+1�����жϼ������Ƿ�>=����N(���)
���� ������������,�򽫱���ֵ�滻��ǰ��Чֵ,���������
���� B���ŵ㣺
���� ���ڱ仯�����ı�������нϺõ��˲�Ч��,
���� �ɱ������ٽ�ֵ�����������ķ�����/����������ʾ������ֵ����
���� C��ȱ�㣺
���� ���ڿ��ٱ仯�Ĳ�������
���� ����ڼ������������һ�β�������ֵǡ���Ǹ���ֵ,��Ὣ����ֵ������Чֵ����ϵͳ
#define N 12
char filter()
{
   char count=0;
   char new_value;
   new_value = get_ad();
   while (value !=new_value);
   {
      count++;
      if (count>=N)   return new_value;
       delay();
      new_value = get_ad();
   }
   return value;    
}

����10���޷������˲���
���� A��������
���� �൱�ڡ��޷��˲�����+�������˲�����
���� ���޷�,������
���� B���ŵ㣺
���� �̳��ˡ��޷����͡����������ŵ�
���� �Ľ��ˡ������˲������е�ĳЩȱ��,���⽫����ֵ����ϵͳ
���� C��ȱ�㣺
  
�� �ο��ӳ���1��9 */

/*
float SetPoint;       //�趨�¶�
float Proportion;     //����ϵ��
float  Integral;      //����ϵ�� 
float Derivative;     //΢��ϵ�� 
float LastError;      // Error[-1] 
float PrevError;      // Error[-2] 
float SumError;       // Sums of Errors 

 // PID�������ͨ��PID��

float PIDCalc( float NextPoint ) 
{ 
	
  float dError=0, Error=0;  
 Error = SetPoint - NextPoint;         
 SumError += Error;                      
 dError = Error - LastError;            
 PrevError = LastError; 
 LastError = Error; 
 return ((Proportion * Error) + (Integral * SumError) + (Derivative * dError) ); 
 
} 

//��ʼ��PID�����
void Init_Pid(void)
{



		 SetPoint =90;       //�趨�¶�90���϶�
		 Proportion =1;     //����ϵ��     �����������������ϵ���Ĵ�С ���������������С�������
		 Integral =0;      //����ϵ�� 
		 Derivative =0;     //΢��ϵ�� 
		 LastError =0;      // Error[-1] 
		 PrevError =0;      // Error[-2] 
		 SumError =0;       // Sums of Errors 




}
  */
 
/*
20160713
float ProcessTcpSrting(char *buf,char*str)
{
	   char *location;
	   uint8_t val[50],i,j,k;
	   float finteger=0,fdecimal=0,fresult=0;

	  location=	strstr(buf,str);
	
	   if(*(location+2)=='=') 
	   {
	   	  for(i=0;i<50;i++)
	   	   {
			    val[i] = *(location+2+i);

				if(val[i]=='.')
				{
				   	    k=i-1;
						j=0;
						finteger=0;
					while(1)
					{
					
						finteger+=(float)((val[k]-0x30)*pow(10,j));
						  j++;
						  k--;
						 if(val[k]=='=')break;
					}
				}

				if(val[i]==0x20)
				{
					   	k=i-1;
						j=0;
					  fdecimal=0;
					  while(1)
					  {	  
					  fdecimal+=(float)((val[k]-0x30)*pow(10,j));
					  k--;
					  j++;
						  if(val[k]=='.')
						  {
						  fdecimal=fdecimal/pow(10,j);	
						  fresult=fdecimal+finteger;
					      return  fresult;
						  }
					  
					  } 
				} 
			}	
	   		 return -1;
	   }







}
*/