close all
clear all
clc

a = [0:255];
expDutyCycles = 256.^(a/255) - 1;

sineWave = 256/2 * sin((2*pi)/128*a) + 256/2;
plot(a, sineWave, a, 256.^(sineWave./255) - 1);
grid on;
