%% Read in the recording from a real highhat
[realhighhat Fs] = audioread('RD_C_HH_3.wav');
sound(realhighhat,Fs); 
realhighhat = 0.5*(realhighhat(:,1) + realhighhat(:,2));

%% Find A_t Function Envelope based on recording 
time = linspace(0,length(realhighhat)/Fs,length(realhighhat));
time = time';
figure(1)
plot(time,realhighhat);
%find peak
[peaks,peakloc] = findpeaks(realhighhat,time,'MinPeakDistance',0.025);
hold on
%find downsampled stem shape
stem(peakloc,peaks);

%keep the plot and find the approximate A_t function
tau = 0.045;
r = 0.3;
T = 1/Fs;
t_r = 0:T:r-T;

%decay
TimeDecay = exp((-(t_r-peakloc(1))/tau));
[~,decayStart_idx] = min(abs(time - peakloc(1)));
TimeDecay = TimeDecay(decayStart_idx:end);

%attack
TimeAttack = linspace(0,peaks(1),length(t_r)-length(TimeDecay));

%combine A and D  for A_t function
A_t = [TimeAttack TimeDecay];
hold on
plot(t_r,A_t);
hold off

%% Add in white noise as the base for highhat sound
dither = randn(1,length(t_r));
%shape the whitenoise with the time-domain envelope
dither = dither.*A_t;
highhatFund = dither*0.2;
sound(highhatFund,Fs)

%% Tune I_t, the freuqncy-domain function envelope
%add in the tone for hihat
I_t = exp(-t_r/0.2);
fc = 350;
fm = 2*fc;
I_0 = 20;
hhtone = A_t.*sin(2*pi*fc*t_r + I_0*I_t.*sin(2*pi*fm*t_r));

%combine the tone with the white-noise base for hihat
%FM synthesized sound
highhat = highhatFund + 0.15*hhtone;
soundsc(highhat,Fs);

%% Frequency spectrum comparison
N = 1024;
zp = 10;
figure(4)
spectrogram(realhighhat,hamming(N),N/2,zp*N,Fs,'yaxis');
title('Real Hihat');
ylim([0 5]);
figure(5)
spectrogram(highhat,hamming(N),N/2,zp*N,Fs,'yaxis');
ylim([0 5]);
title('FM Hihat');
