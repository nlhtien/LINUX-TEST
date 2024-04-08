#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <math.h>

int fd;

#define MPU6050_REG_ACC_X_HIGH          0x3B // acc
#define MPU6050_REG_GYR_X_HIGH          0x43 // gyro

#define MPU6050_ACCEL_CONFIG        0x1C //register 28
#define MPU6050_GYRO_CONFIG         0x1B //register 27

#define MPU6050_SLAVE_ADDR 0x68
#define MPU6050_POWER 0x6B

static void mpu6050_write(uint8_t addr, uint8_t value){
    uint8_t buf[2];
    buf[0]= addr;
    buf[1]=value;
    int ret;
    if((ret = write(fd,buf,2))<=0){
        printf("WRITE FAILED!!! \n");
    }

    usleep(500);

}

void mpu6050_init(){
    // Turn off sleep mode of MPU6050
    mpu6050_write(MPU6050_POWER, 0x00);

    //Acclerator Configuration
    mpu6050_write(MPU6050_ACCEL_CONFIG, 0x08); //=> Full scale range = +-4g

    //Gyroscope Configuration
    mpu6050_write(MPU6050_GYRO_CONFIG, 0x10); // => Full scale range = +-1000 degree/second


}

void mpu6050_read(uint8_t addr, int* buf){
    int ret;
    char read_buffer[6];
    for(int i=0;i<6;i++){
        ret = write(fd, &addr,1);
        if(ret <=0)
            printf("Address Read failed(0)\n");
        ret = read(fd,&read_buffer[i],1);
        if(ret <=0)
            printf("Address Read failed(1)\n");
        addr++;
    }

    buf[0] = ((int)(read_buffer[0]<<8) | (int)read_buffer[1]);
    buf[1] = ((int)(read_buffer[2]<<8) | (int)read_buffer[3]);
    buf[2] = ((int)(read_buffer[4]<<8) | (int)read_buffer[5]);

    usleep(500);

}

// Convert radians to degrees
float rad_to_deg(float rad) {
    return rad * 180.0 / M_PI;
}

// Convert degrees to radians
float deg_to_rad(float deg) {
    return deg * M_PI / 180.0;
}

// Calculate roll and pitch from accelerometer data
void calculate_roll_pitch(int* acc, float* roll, float* pitch) {
    *roll = atan2(acc[1], acc[2]);
    *pitch = atan2(-acc[0], sqrt(acc[1]*acc[1] + acc[2]*acc[2]));
}


float calculate_yaw(int* gyro, float dt, float previous_yaw) {
    static float yaw = 0;
    float gy = deg_to_rad(gyro[2] / 131.0);
    yaw += gy * dt;
    return previous_yaw * 0.98 + yaw * 0.02;
}

int main(){

    int acc[3], gyro[3];

    //Open the i2c device file
    if((fd=open("/dev/i2c-1",O_RDWR))<0){
        printf("Cant Open I2C file\n");
        return -1;
    }

    if (ioctl(fd,I2C_SLAVE,MPU6050_SLAVE_ADDR) < 0) { //raspberry pi 3+ as Master and MPU6050 device as slave
        printf("Failed to set I2C slave address.\n");
        close(fd);
        return -1;
    }


    mpu6050_init();

    float roll, pitch, yaw = 0;
    float dt = 0.01;

    while(1){

        mpu6050_read(MPU6050_REG_ACC_X_HIGH, acc);
        mpu6050_read(MPU6050_REG_GYR_X_HIGH, gyro);

        calculate_roll_pitch(acc, &roll, &pitch);
        yaw = calculate_yaw(gyro, dt, yaw);

        // Convert roll, pitch, and yaw from radians to degrees
        roll = rad_to_deg(roll);
        pitch = rad_to_deg(pitch);
        yaw = rad_to_deg(yaw);

        printf("Roll = %.2f, Pitch = %.2f, Yaw = %.2f\n", roll, pitch, yaw);

        usleep(500000); // Wait for 0.5 seconds
    }

    return 0;
}
