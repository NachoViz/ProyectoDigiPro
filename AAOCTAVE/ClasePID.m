

close all; clear all; clc;

s = tf('s')

G= zpk([-40],[ -10+5i -10-5i ],18)

% Condiciones de diseño SO de 0% y error en estado estable 0
pole(G)
% La funcion de transferencia del PID es kp*(ti*td*s^2 + ti*s + 1)/(ti*s)

% ti*td*kp*(s^2+1/td*s+1/ti*td)/(ti*s)
% td*kp*(s^2+1/td*s+1/ti*td)/(s)
% s^2+1/td*s+1/ti*td)=s^2+20*s+125

td=1/20
ti=1/(td*125)

pid=(ti*td*s^2 + ti*s + 1)/(ti*s)

FdA=pid*G

pzmap(FdA)

rlocusx(FdA)
kp=3.43

pidFinal=kp*(ti*td*s^2 + ti*s + 1)/(ti*s)

FdT=feedback(pidFinal*G ,1)