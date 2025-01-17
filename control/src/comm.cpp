#include "comm.h"
#include "pid.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

/// @brief 打表代码：使用polynomial生成CRC-8的所有计算结果
/// @param polynomial 指定生成多项式 
void GenerateCrc8Table(uint8_t polynomial)  
{  
    uint8_t crc=0;
    uint16_t i,j;
    for(j = 0;j<256;j++)
    {
        if(!(j%16))
            printf("\r\n");
 
        crc = (uint8_t)j;
        for (i = 0; i < 8; i++)             
        {  
            if (crc & 0x80)
                crc = (crc << 1) ^ polynomial;
            else
                crc <<= 1;                    
        }
        printf("0x%02x,",crc);		
    }
    printf("\b \r\n");
}
/// @brief 使用0x07生成的表
const uint8_t crc_table[] = {
    0x00,0x07,0x0e,0x09,0x1c,0x1b,0x12,0x15,0x38,0x3f,0x36,0x31,0x24,0x23,0x2a,0x2d,
    0x70,0x77,0x7e,0x79,0x6c,0x6b,0x62,0x65,0x48,0x4f,0x46,0x41,0x54,0x53,0x5a,0x5d,
    0xe0,0xe7,0xee,0xe9,0xfc,0xfb,0xf2,0xf5,0xd8,0xdf,0xd6,0xd1,0xc4,0xc3,0xca,0xcd,
    0x90,0x97,0x9e,0x99,0x8c,0x8b,0x82,0x85,0xa8,0xaf,0xa6,0xa1,0xb4,0xb3,0xba,0xbd,
    0xc7,0xc0,0xc9,0xce,0xdb,0xdc,0xd5,0xd2,0xff,0xf8,0xf1,0xf6,0xe3,0xe4,0xed,0xea,
    0xb7,0xb0,0xb9,0xbe,0xab,0xac,0xa5,0xa2,0x8f,0x88,0x81,0x86,0x93,0x94,0x9d,0x9a,
    0x27,0x20,0x29,0x2e,0x3b,0x3c,0x35,0x32,0x1f,0x18,0x11,0x16,0x03,0x04,0x0d,0x0a,
    0x57,0x50,0x59,0x5e,0x4b,0x4c,0x45,0x42,0x6f,0x68,0x61,0x66,0x73,0x74,0x7d,0x7a,
    0x89,0x8e,0x87,0x80,0x95,0x92,0x9b,0x9c,0xb1,0xb6,0xbf,0xb8,0xad,0xaa,0xa3,0xa4,
    0xf9,0xfe,0xf7,0xf0,0xe5,0xe2,0xeb,0xec,0xc1,0xc6,0xcf,0xc8,0xdd,0xda,0xd3,0xd4,
    0x69,0x6e,0x67,0x60,0x75,0x72,0x7b,0x7c,0x51,0x56,0x5f,0x58,0x4d,0x4a,0x43,0x44,
    0x19,0x1e,0x17,0x10,0x05,0x02,0x0b,0x0c,0x21,0x26,0x2f,0x28,0x3d,0x3a,0x33,0x34,
    0x4e,0x49,0x40,0x47,0x52,0x55,0x5c,0x5b,0x76,0x71,0x78,0x7f,0x6a,0x6d,0x64,0x63,
    0x3e,0x39,0x30,0x37,0x22,0x25,0x2c,0x2b,0x06,0x01,0x08,0x0f,0x1a,0x1d,0x14,0x13,
    0xae,0xa9,0xa0,0xa7,0xb2,0xb5,0xbc,0xbb,0x96,0x91,0x98,0x9f,0x8a,0x8d,0x84,0x83,
    0xde,0xd9,0xd0,0xd7,0xc2,0xc5,0xcc,0xcb,0xe6,0xe1,0xe8,0xef,0xfa,0xfd,0xf4,0xf3
};
/// @brief CRC-8查表算法
/// @param data 多字节校验数据
/// @param length 字节长度
/// @return 校验码
uint8_t crc8(uint8_t* data, size_t length) {
    uint8_t crc = INITIAL_REMAINDER;
    while(length--){
        crc = crc_table[crc ^ *data++];
    }
    return crc;
}

// 海明编码函数
uint16_t hammingEncode(uint16_t data) {
    uint16_t code = 0;
    // 放置数据位
    code |= (data & 0x1) << 2; // D1
    code |= (data & 0xE) << 3; // D2-D4, 二进制的1110等于十六进制的E
    code |= (data & 0x7F0) << 4; // D5-D11, 二进制的11111110000等于十六进制的7F0

    cout << "code init: " << code << endl;
    // 计算校验位
    uint8_t p1 = ((code >> 2) & 1) ^ ((code >> 4) & 1) ^ ((code >> 6) & 1) ^ ((code >> 8) & 1) ^ ((code >> 10) & 1) ^ ((code >> 12) & 1) ^ ((code >> 14) & 1);
    uint8_t p2 = ((code >> 2) & 1) ^ ((code >> 5) & 1) ^ ((code >> 6) & 1) ^ ((code >> 9) & 1) ^ ((code >> 10) & 1) ^ ((code >> 13) & 1) ^ ((code >> 14) & 1);
    uint8_t p4 = ((code >> 4) & 1) ^ ((code >> 5) & 1) ^ ((code >> 6) & 1) ^ ((code >> 11) & 1) ^ ((code >> 12) & 1) ^ ((code >> 13) & 1) ^ ((code >> 14) & 1);
    uint8_t p8 = ((code >> 8) & 1) ^ ((code >> 9) & 1) ^ ((code >> 10) & 1) ^ ((code >> 11) & 1) ^ ((code >> 12) & 1) ^ ((code >> 13) & 1) ^ ((code >> 14) & 1);

    // 设置校验位
    code |= (p1 & 1) << 0;
    code |= (p2 & 1) << 1;
    code |= (p4 & 1) << 3;
    code |= (p8 & 1) << 7;

    return code;
}

/**
 * @brief 编码器
 * @note  将上位机数据参考编码方案编码并发送给下位机 
 * @note  下位机速度控制范围:  +- 31
 * @note  舵机控制范围:       +- 1200
*/
void encode_and_send(void)
{
	vector<unsigned char> uart_data;
	uint16_t speed_result_tmp;
	uint16_t angle_result_tmp;
    unsigned char speed_crc = 0, angle_crc = 0, angle_crc_p2 = 0;
    unsigned char crc_data[2];
    unsigned char speed_code[3],servo_code[3];

    std::chrono::time_point<std::chrono::high_resolution_clock> uart_trans_begin_ts = std::chrono::high_resolution_clock::now();

	//使用18th的[0,250]调出来的pid，拓展到+-1250
	speed_result_tmp = ENC_SPEED_SCALE * (speed_result < 0 ? -speed_result : speed_result);
    if(disable_motor)
    {
        crc_data[0] = (777 >> 8);
        crc_data[1] = 777 & 0xff;
        speed_crc = crc8(crc_data,2);
        speed_result_tmp = hammingEncode(777);
        speed_code[0] = (speed_result_tmp >> 9);
	    speed_code[1] = (speed_result_tmp & 0x1f8) >> 3;
        speed_code[2] = (speed_result_tmp & 0x7) << 3;
        if(speed_crc & 0x80)
        {
            speed_code[2] |= 0x4;
        }
        speed_code[2] |= speed_crc >> 6;
        speed_code[3] = speed_crc & 0x3f;
        // cout << "disable_motor" << endl;
    }
    else if(direct_motor_power_ctrl)
    {
        speed_result_tmp += 900;
        if(speed_result < 0) speed_result_tmp |= 0x400;
        crc_data[0] = (speed_result_tmp >> 8);
        crc_data[1] = speed_result_tmp & 0xff;
        speed_crc = crc8(crc_data,2);
        speed_result_tmp = hammingEncode(speed_result_tmp);
        speed_code[0] = (speed_result_tmp >> 9);
	    speed_code[1] = (speed_result_tmp & 0x1f8) >> 3;
        speed_code[2] = (speed_result_tmp & 0x7) << 3;
        if(speed_crc & 0x80)
        {
            speed_code[2] |= 0x4;
        }
        speed_code[2] |= speed_crc >> 6;
        speed_code[3] = speed_crc & 0x3f;
    }
    else
    {
        if(speed_result_tmp > MAX_SPEED_VAL)   speed_result_tmp = MAX_SPEED_VAL;
        if(speed_result < 0) speed_result_tmp |= 0x400;
        cout << "data in: " << speed_result_tmp << endl;
        crc_data[0] = (speed_result_tmp >> 8);
        crc_data[1] = speed_result_tmp & 0xff;
        speed_crc = crc8(crc_data,2);   
        speed_result_tmp = hammingEncode(speed_result_tmp);
        speed_code[0] = (speed_result_tmp >> 9);
	    speed_code[1] = (speed_result_tmp & 0x1f8) >> 3;
        speed_code[2] = (speed_result_tmp & 0x7) << 3;
        if(speed_crc & 0x80)
        {
            speed_code[2] |= 0x4;
        }
        speed_code[2] |= speed_crc >> 6;
        speed_code[3] = speed_crc & 0x3f;
    }
    
	angle_result_tmp = angle_result < 0 ? -angle_result : angle_result;
	if(angle_result_tmp > MAX_ANGLE_VAL) angle_result_tmp = MAX_ANGLE_VAL;

    if(angle_result < 0) angle_result_tmp |= 0x400;
    crc_data[0] = (angle_result_tmp >> 8);
    crc_data[1] = angle_result_tmp & 0xff;
    angle_crc = crc8(crc_data,2);
    angle_result_tmp = hammingEncode(angle_result_tmp);
    servo_code[0] = (angle_result_tmp >> 9);
    servo_code[1] = (angle_result_tmp & 0x1f8) >> 3;
    servo_code[2] = (angle_result_tmp & 0x7) << 3;
    if(angle_crc & 0x80)
    {
        servo_code[2] |= 0x4;
    }
    servo_code[2] |= angle_crc >> 6;
    servo_code[3] = angle_crc & 0x3f;


    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);
    uart_data.push_back(BATCH_TRANS_BEGIN);

	for(int i = 0; i <= ANGLE_TRANS_COUNT; i++)
	{
        uart_data.push_back(SINGEL_TRANS_BEGIN); //舵机传输开始标志
        uart_data.push_back(servo_code[0]);
        uart_data.push_back(servo_code[1]);
        uart_data.push_back(servo_code[2]);
        uart_data.push_back(servo_code[3]);
        // uart_data.push_back(angle_crc);
        // uart_data.push_back(TRANS_OVER);
	}
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    uart_data.push_back(TRANS_SPLIT);
    
	for(int i = 0; i <= SPEED_TRANS_COUNT; i++)
	{
        uart_data.push_back(SINGEL_TRANS_BEGIN); //速度传输开始标志
        uart_data.push_back(speed_code[0]);
        uart_data.push_back(speed_code[1]);
        uart_data.push_back(speed_code[2]);
        uart_data.push_back(speed_code[3]);
        // uart_data.push_back(speed_crc);
        // uart_data.push_back(TRANS_OVER);
    }
	ser.write(uart_data);

	std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - start_time_stamp;
    std::chrono::duration<double, std::milli> duration = std::chrono::high_resolution_clock::now() - uart_trans_begin_ts;
    cout << "UART time consumed: " << duration.count() << "ms" <<endl;
	cout << "now: " << elapsed.count() << "ms" <<endl;
	cout << "Angle: " << dec << angle_result  << "	Speed: " << speed_result<< endl;
	cout << "************************************************************************"<< endl;
	cout << "HEX:   Angle: "  << hex << (uint)angle_result_tmp << "	Speed: " << (uint)angle_result_tmp <<  endl;
    cout << "HEX:   Angle CRC: "  << hex << (uint)angle_crc  << "	Speed CRC: " << (uint)speed_crc <<  endl;
	cout << "************************************************************************"<< dec << endl;
}

int get_speed_enc(void)
{
    vector<comm_data_t> speed_vals;
	string data;
    static int last_real_speed_enc;
	int real_speed_enc_tmp;
    int cnt = 0;
    speed_vals.resize(1);
	if(ser.available())
	{
		data = ser.read(ser.available()); // 读取串口数据
		
		for (auto i = data.size() - 1; i >= 0 ; i-- )
		{
			if((data[i] & 0xc0) == 0xc0)
			{
				if((data[i-1] & 0x40) == 0x40)
				{
					real_speed_enc_tmp = (data[i-1] & 0x1f) << 6;
					real_speed_enc_tmp |= data[i] & 0x3f;
					if((data[i-1] & 0x20) == 0x20)
					{
						real_speed_enc_tmp = -real_speed_enc_tmp;
					}
                    bool speed_found_eq = false;
                    if(speed_vals.size() > 1)
                    {
                        for(auto k = 0; k < speed_vals.size(); k++)
                        {
                            if(real_speed_enc_tmp == speed_vals[k].val)
                            {
                                speed_vals[k].cnt++;
                                speed_found_eq = true;
                                if(k >= 1)
                                {
                                    for(int j = 0; j < k; j++)
                                    {
                                        comm_data_t tmp;
                                        if( speed_vals[k].cnt > speed_vals[j].cnt)
                                        {
                                            tmp = speed_vals[j];
                                            speed_vals[j] = speed_vals[k];
                                            speed_vals[k] = tmp;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        if(!speed_found_eq)
                        {
                            comm_data_t tmp;
                            tmp.val = real_speed_enc_tmp;
                            tmp.cnt = 1;
                            speed_vals.push_back(tmp);
                        }
                    }
                    else
                    {
                        if(speed_vals[0].val != 0)
                        {
                            if(real_speed_enc_tmp == speed_vals[0].val)
                            {
                                speed_vals[0].cnt++;
                            }
                            else
                            {
                                comm_data_t tmp;
                                tmp.val = real_speed_enc_tmp;
                                tmp.cnt = 1;
                                speed_vals.push_back(tmp);
                            }
                        }
                        else
                        {
                            speed_vals[0].val = real_speed_enc_tmp;
                            speed_vals[0].cnt = 1;
                        }
                    }
                    // i--;	
				}
			}
            cnt ++;
            if(cnt > ENC_SPEED_COUNT)
            {
                break;
            }
		}
        if(abs(last_real_speed_enc - speed_vals[0].val) > 150 || (abs(last_real_speed_enc) > 80 &&  speed_vals[0].val == 0))
        {
            return last_real_speed_enc;
        }  
        else
        {
            last_real_speed_enc = speed_vals[0].val;
            return speed_vals[0].val;
        }
        
	}
    else 
    {
        return last_real_speed_enc;
    }
}