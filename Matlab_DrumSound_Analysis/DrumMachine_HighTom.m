%% Read in the recording from a real HighTom
[realhightom Fs] = audioread('RD_T_HT_3.wav');
sound(realhightom,Fs); 
realhightom = 0.5*(realhightom(:,1) + realhightom(:,2));

%% Find A_t Function Envelope based on recording 
time = linspace(0,length(realhightom)/Fs,length(realhightom));
time = time';
figure(1)
plot(time,realhightom);
%find peak
[peaks,peakloc] = findpeaks(realhightom,time,'MinPeakDistance',0.025);
hold on
%find downsampled stem shape
stem(peakloc,peaks);

%keep the plot and find the approximate A_t function
tau = 0.1;
A = 0.999;
r = 0.6;
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

%% Tune I_t, the frequency-domain function envelope
figure(2)
%tune the ring tone for the hightom
tau = 100;
R = 18500*(t_r+0.01).^2.*exp(-tau*(t_r+0.01));
I_t = R;
plot(I_t);
fc = 200;
fm = 400;
I_0 = 1.5;
hightom_ring = A_t.*sin(2*pi*fc*t_r + I_0*I_t.*sin(2*pi*fm*t_r));
sound(hightom_ring,Fs);

%% tune the bouncy percussive sound for the hightom
r2 = 0.03;
t_r2 = 0:T:r2-T;
R2 = -(1/r2)*t_r2 + 1;
A_t2 = R2;
I_t2 = R2;

fc2 = 200;
fm2 = 350;
I0_2 = 5;
stick= A_t2.*sin(2*pi*fc2*t_r2 + I0_2*I_t2.*sin(2*pi*fm2*t_r2));

stick = [stick zeros(1,length(hightom_ring)-length(stick))];
sound(stick,Fs);

%% add the ring-tone and the bouncy percussion together
%FM synthesized sound
hightom = hightom_ring + stick;
soundsc(hightom,Fs);

%% Frequency spectrum comparison
N = 1024;
zp = 10;
figure(4)
spectrogram(realhightom,hamming(N),N/2,zp*N,Fs,'yaxis');
title('Real HighTom');
ylim([0 5]);
figure(5)
spectrogram(hightom,hamming(N),N/2,zp*N,Fs,'yaxis');
title('FM HighTom');
ylim([0 5]);
