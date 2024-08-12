%% Read in the recording from a real KickDrum
[realkick Fs] = audioread('RD_K_5.wav');
sound(realkick,Fs); 
realkick = 0.5*(realkick(:,1) + realkick(:,2));

%% Find A_t Function Envelope based on recording 
time = linspace(0,length(realkick)/Fs,length(realkick));
time = time';
figure(1)
plot(time,realkick);

%find peak
[peaks,peakloc] = findpeaks(realkick,time,'MinPeakDistance',0.025);
hold on
%find downsampled stem shape
stem(peakloc,peaks);

%keep the plot and find the approximate A_t function
tau = 0.065;
A = 0.999;
r = 0.30;
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
tau = 0.1;
R  = -(1/70)*t_r + 1;
I_t = R;
plot(t_r,I_t);
fc = 70;
fm = 30;
I_0 = 1.15;
kick = A_t.*sin(2*pi*fc*t_r + I_0*I_t.*sin(2*pi*fm*t_r));
sound(kick,Fs)

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

stick = [stick zeros(1,length(kick)-length(stick))];
sound(stick,Fs);
%% add together; dither the sound a bit
%FM synthesized sound
dither = randn(1,length(t_r));
dither = dither.*A_t*0.001;
kickFM = kick+dither+stick*0.05;
sound(kickFM,Fs);

%% Frequency spectrum comparison
N = 1024;
zp = 10;
figure(3);
spectrogram(realkick,hamming(N),N/2,zp*N,Fs,'yaxis');
ylim([0 5]);
title('Real Kick');
figure(4);
spectrogram(kickFM,hamming(N),N/2,zp*N,Fs,'yaxis');
ylim([0 5]);
title('FM Kick');

