%% Read in the recording from a real FloorTom
[realfloortom Fs] = audioread('RD_T_FT_4.wav');
sound(realfloortom,Fs); 
realfloortom = 0.5*(realfloortom(:,1) + realfloortom(:,2));

%% Find A_t Function Envelope based on recording 
time = linspace(0,length(realfloortom)/Fs,length(realfloortom));
time = time';
figure(1)
plot(time,realfloortom);
%find peak
[peaks,peakloc] = findpeaks(realfloortom,time,'MinPeakDistance',0.025);
hold on
%find downsampled stem shape
stem(peakloc,peaks);

%keep the plot and find the approximate A_t function
tau = 0.15;
A = 0.6;
r = 0.55;
T = 1/Fs;
t_r = 0:T:r-T;

%decay
TimeDecay = A*exp((-(t_r-peakloc(1))/tau));
[~,decayStart_idx] = min(abs(time - peakloc(1)));
TimeDecay = TimeDecay(decayStart_idx:end);

%attack
TimeAttack = linspace(0,peaks(1),length(t_r)-length(TimeDecay));

%combine A and D  for A_t function
A_t = [TimeAttack TimeDecay];
hold on
plot(t_r,A_t);
hold off

%% Tune I_t, the freuqncy-domain function envelope

%According the spectrogram, there's a high frequency tone from the floor tom
figure(2)
tau = 2.5;
R = -2*t_r(1:round((1/3)*length(A_t)))+1;
I_t = R;
plot(I_t);
fc = 600;
fm = 0;
I_0 = 1;
floortom_highF = A_t(1:length(R)).*sin(2*pi*fc*t_r(1:length(R)) + I_0*I_t.*sin(2*pi*fm*t_r(1:length(R))));
sound(floortom_highF,Fs);
floortom_highF = [floortom_highF zeros(1,length(A_t)-length(floortom_highF))];

%% tune the ring tone for the floortom
figure(3)
tau2 = 2.5;
R2 = -2*t_r+1;
I_t2 = R2;
plot(I_t2);
fc2 = 90;
fm2 = 50;
I_02 = 2;
floortom_ring = A_t.*sin(2*pi*fc2*t_r + I_02*I_t2.*sin(2*pi*fm2*t_r));
sound(floortom_ring,Fs);

%% tune the bouncy percussive sound for the hightom
r3 = 0.1;
T = 1/Fs;
t_r3 = 0:T:r3-T;
R3 = -(1/r3)*t_r3 + 1;
A_t3 = R3;
I_t3 = R3;

fc3 = 100;
fm3 = 80;
I_03 = 5;
stick= A_t3.*sin(2*pi*fc3*t_r3 + I_03*I_t3.*sin(2*pi*fm3*t_r3));

%zeropad to ensure all sounds have the same length
stick = [stick zeros(1,length(floortom_highF)-length(stick))];
sound(stick,Fs);

%% combine the 3 sounds
%FM synthesized sound
floortom = 0.01*floortom_highF + stick + floortom_ring;
sound(floortom,Fs);

%% Frequency spectrum comparison
N = 1024;
zp = 10;
figure(4)
spectrogram(realfloortom,hamming(N),N/2,zp*N,Fs,'yaxis');
title('Real FloorTom');
ylim([0 5]);

figure(5)
spectrogram(floortom,hamming(N),N/2,zp*N,Fs,'yaxis');
title('FM FloorTom');
ylim([0 5]);
