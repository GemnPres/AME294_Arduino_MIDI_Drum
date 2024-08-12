%% Read in the recording from a real Snare
[realsnare Fs] = audioread('RD_S_1.wav');
sound(realsnare,Fs); 
realsnare = 0.5*(realsnare(:,1) + realsnare(:,2));

%% Find A_t Function Envelope based on recording 
time = linspace(0,length(realsnare)/Fs,length(realsnare));
time = time';
figure(1)
plot(time,realsnare);
%find peak
[peaks,peakloc] = findpeaks(realsnare,time,'MinPeakDistance',0.025);
hold on
%find downsampled stem shape
stem(peakloc,peaks);

%keep the plot and find the approximate A_t function
tau = 0.04;
A = 0.999;
r = 0.25;
T = 1/Fs;
t_r = 0:T:r-T;

%decay
TimeDecay = A*exp((-(t_r-peakloc(1))/tau));
[~,decayStart_idx] = min(abs(time - peakloc(1)));
TimeDecay = TimeDecay(decayStart_idx:end);

%attack
TimeAttack = linspace(0,peaks(1),length(t_r)-length(TimeDecay));

%combine A and D for A_t function
A_t = [TimeAttack TimeDecay];
hold on
plot(t_r,A_t);
hold off

%% Tune I_t, the frequency-domain function envelope
figure(2)
%tune the body part of the snare sound
tau = 0.03;
R  = exp(-t_r/tau);
I_t = R;
plot(I_t);
fc = 80;
fm = 85;
I_0 = 1;
snareFund = A_t.*sin(2*pi*fc*t_r + I_0*I_t.*sin(2*pi*fm*t_r));
soundsc(snareFund,Fs);

%% add in the snare part of the snare sound
dither = randn(1,length(t_r));
dither = dither.*A_t*0.035;
snare = snareFund+dither;
soundsc(snare,Fs)

%% Frequency spectrum comparison

N = 1024;
zp = 10;
figure(3)
spectrogram(realsnare,hamming(N),N/2,zp*N,Fs,'yaxis');
ylim([0 5]);
title("Real Snare");
figure(4)
spectrogram(snare,hamming(N),N/2,zp*N,Fs,'yaxis');
ylim([0 5]);
title("FM Snare");

