
#include "comm.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

uint8_t crc8(uint8_t *data, size_t length) 
{
    uint8_t crc = INITIAL_REMAINDER;
    for (size_t byte = 0; byte < length; ++byte) {
        crc ^= data[byte]; // XOR byte into least sig. byte of crc

        for (uint8_t bit = 8; bit > 0; --bit) { // Loop over each bit
            if (crc & 0x80) { // If the uppermost bit is 1...
                crc = (crc << 1) ^ POLYNOMIAL; // Shift left and XOR with the polynomial
            } else {
                crc <<= 1; // Just shift left
            }
        }
    }

    // Note: XOR with 0x00 can be omitted as it does not change the result
    return crc; // Final remainder is the CRC result
}

void encode_and_send(void)
{
	vector<unsigned char> speed_data;
    vector<unsigned char> angle_data;
	unsigned char speed_result_tmp;
	uint angle_result_tmp = 0;
    unsigned char speed_crc = 0, angle_crc = 0, angle_crc_p2 = 0;
    unsigned char crc_data[2];
	//使用18th的[0,250]调出来的pid，拓展到+-1250
	speed_result_tmp = speed_result < 0 ? -speed_result : speed_result;
    if(speed_result_tmp > 22)   speed_result_tmp = 22;
	angle_result_tmp = angle_result < 0 ? -angle_result : angle_result;

	if(angle_result_tmp > 1200) angle_result_tmp = 1200;
	unsigned char speed_code,servo_code_p1,servo_code_p2;
	speed_code = speed_result_tmp & 0x1f;
	if(speed_result < 0) speed_code |= 0x20;
	servo_code_p1 = (angle_result_tmp >> 6) & 0x1f;
	if(angle_result < 0) servo_code_p1 |= 0x20;
	servo_code_p1 |= 0x80;
	servo_code_p2 = angle_result_tmp & 0x3f;
	servo_code_p2 |= 0xc0;
    
    crc_data[0] = speed_code;
    speed_crc = crc8(crc_data,1);
    crc_data[0] = servo_code_p1;
    crc_data[1] = servo_code_p2;
    angle_crc = crc8(crc_data,2);
    // angle_crc_p2 = crc8(&servo_code_p2,1);
    angle_data.push_back(0xff); 
	angle_data.push_back(0x55); //舵机传输开始标志
	angle_data.push_back(0x55); //舵机传输开始标志
	for(int i = 0; i < 5; i++)
	{
		angle_data.push_back(servo_code_p1);
		angle_data.push_back(servo_code_p2);
        angle_data.push_back(0x6f); ///angle crc传输标志
        angle_data.push_back(angle_crc);
        angle_data.push_back(0x6f); ///angle crc传输标志
        angle_data.push_back(angle_crc);
        angle_data.push_back(0x6f); ///angle crc传输标志
        angle_data.push_back(angle_crc);
		angle_data.push_back(0x5b); //传输分割标志
		angle_data.push_back(0x5b); //传输分割标志
	}
    angle_data.push_back(servo_code_p1);
    angle_data.push_back(servo_code_p2);
    angle_data.push_back(0x6f); ///angle crc传输标志
    angle_data.push_back(angle_crc);
    angle_data.push_back(0x6f); ///angle crc传输标志
    angle_data.push_back(angle_crc);
    angle_data.push_back(0x6f); ///angle crc传输标志
    angle_data.push_back(angle_crc);
    angle_data.push_back(0x5c); //舵机传输结束标志
    angle_data.push_back(0x5c); //舵机传输结束标志
    angle_data.push_back(0xff);
    ser.write(angle_data);

    angle_data.push_back(0xff);
	speed_data.push_back(0x6b); //速度传输开始标志
	speed_data.push_back(0x6b); //速度传输开始标志
	for(int i = 0; i < 5; i++)
	{
        speed_data.push_back(speed_code);
        speed_data.push_back(0x6c); //speed crc传输标志
        speed_data.push_back(speed_crc);
        speed_data.push_back(0x6c);
        speed_data.push_back(speed_crc);
        speed_data.push_back(0x6c);
        speed_data.push_back(speed_crc);
		speed_data.push_back(0x5b); //传输分割标志
		speed_data.push_back(0x5b); //传输分割标志
    }
    speed_data.push_back(speed_code);
    speed_data.push_back(0x6c); //speed crc传输标志
    speed_data.push_back(speed_crc);
    speed_data.push_back(0x6c);
    speed_data.push_back(speed_crc);
    speed_data.push_back(0x6c);
    speed_data.push_back(speed_crc);
	speed_data.push_back(0x6a);	//速度传输结束标志
	speed_data.push_back(0x6a);	//速度传输结束标志
    angle_data.push_back(0xff);

	ser.write(speed_data);
	std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - start_time_stamp;
	cout << "now: " << elapsed.count() << "ms" <<endl;
	cout << "Angle: " << dec << angle_result  << "	Speed: " << (int)speed_result<< endl;
	cout << "************************************************************************"<< endl;
	cout << "HEX:   Angle: "  << hex << (uint)servo_code_p1  << (uint)servo_code_p2 << "	Speed: " << (uint)speed_code <<  endl;
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
            if(cnt > 10)
            {
                break;
            }
		}
        last_real_speed_enc = speed_vals[0].val;
        return speed_vals[0].val;
	}
    else 
    {
        return last_real_speed_enc;
    }
}