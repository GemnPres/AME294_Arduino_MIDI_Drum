%% Read in the recording from a real Ride
[realride Fs] = audioread('RD_C_R_4.wav');
realride = 0.5*(realride(:,1) + realride(:,2));
sound(realride,Fs);
%% Find A_t Function Envelope based on recording 
time = linspace(0,length(realride)/Fs,length(realride));
time = time';
figure(1)
plot(time,realride);
%find peak
[peaks,peakloc] = findpeaks(realride,time,'MinPeakDistance',0.025);
hold on
%find downsampled stem shape
stem(peakloc,peaks);

%keep the plot and find the approximate A_t function
tau = 0.65;
r = 11;
T = 1/Fs;
t_r = 0:T:r-T;
TimeDecay = exp((-(t_r-peakloc(1))/tau));
[~,decayStart_idx] = min(abs(time - peakloc(1)));
TimeDecay = TimeDecay(decayStart_idx:end);

%decay
TimeAttack = linspace(0,peaks(1),length(t_r)-length(TimeDecay));

%attack
A_t = [TimeAttack TimeDecay];
hold on
plot(t_r,A_t);
hold off

%% Tune I_t, the frequency-domain function envelope

%add in noise, body part of the ride
dither = randn(1,length(t_r));
dither = dither.*A_t;
rideFund = dither*0.025;
sound(rideFund,Fs);

%% I_t; tune the tone for the ride
I_t = exp(-t_r/0.85);
fc = 790;
fm = 1.2*fc;
I_0 = 10;
ridetone = A_t.*sin(2*pi*fc*t_r + I_0*I_t.*sin(2*pi*fm*t_r));
%add together
%FM synthesized rider sound
ride = 0.3*rideFund + 0.1*ridetone;
ride = 1.5*ride;
sound(ride,Fs);


%% Spectrogram comparison
N = 1024;
zp = 10;
figure(4)
spectrogram(realride,hamming(N),N/2,zp*N,Fs,'yaxis');
hold on
title("Real Rider")
plot(t_r,I_0.*I_t);
figure(5)
spectrogram(ride,hamming(N),N/2,zp*N,Fs,'yaxis');
title("FM Rider")
