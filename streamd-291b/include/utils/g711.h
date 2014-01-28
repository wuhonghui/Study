/*
* Copyright (c) 2003,北京捷泰科电子技术有限责任公司研发部
* All rights reserved.
* 
* Filename：g711.h
* 文件标识：见配置管理计划书
* 摘    要：G.711 Coder and Decoder，u-law, A-law and linear PCM conversions.
* 
* 当前版本：1.1
* 作    者：DengBaokuan
* 完成日期：April 08, 2003
* 取代版本：1.0 
* 原作者  ：from H.323
* 完成日期：December 30, 1994*/
static int ulaw2alaw ( int );
static int alaw2ulaw ( int);
int ulaw2linear( int );
int linear2ulaw( int );
int alaw2linear(int	);	
int linear2alaw(int	);

