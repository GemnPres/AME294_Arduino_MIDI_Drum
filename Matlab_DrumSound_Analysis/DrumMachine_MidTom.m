%% Read in the recording from a real HighTom
[realmidtom Fs] = audioread('RD_T_MT_3.wav');
sound(realmidtom,Fs); 
realmidtom = 0.5*(realmidtom(:,1) + realmidtom(:,2));

%% Find A_t Function Envelope based on recording 
time = linspace(0,length(realmidtom)/Fs,length(realmidtom));
time = time';
figure(1)
plot(time,realmidtom);
%find peak
[peaks,peakloc] = findpeaks(realmidtom,time,'MinPeakDistance',0.025);
hold on
%find downsampled stem shape
stem(peakloc,peaks);

%keep the plot and find the approximate A_t function
tau = 0.15;
A = 0.999;
r = 0.8;
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
%midtom sounds like hightom but lower in pitch
figure(2)
tau = 70;
R = 18500*(t_r+0.01).^2.*exp(-tau*(t_r+0.01));
I_t = R;
plot(I_t);
fc = 110;
fm = 113;
I_0 = 1.5;
midtom_ring = A_t.*sin(2*pi*fc*t_r + I_0*I_t.*sin(2*pi*fm*t_r));
sound(midtom_ring,Fs);

%% percussive sound
r2 = 0.03;
t_r2 = 0:T:r2-T;
R2 = -(1/r2)*t_r2 + 1;
A_t2 = R2;
I_t2 = R2;

fc2 = 200;
fm2 = 350;
I0_2 = 5;
stick= A_t2.*sin(2*pi*fc2*t_r2 + I0_2*I_t2.*sin(2*pi*fm2*t_r2));

stick = [stick zeros(1,length(midtom_ring)-length(stick))];
sound(stick,Fs);

%% add the 2 sounds
midtom = midtom_ring + stick ;

soundsc(midtom,Fs);

%% Frequency spectrum comparison
N = 1024;
zp = 10;
figure(4)
spectrogram(realmidtom,hamming(N),N/2,zp*N,Fs,'yaxis');
title('Real MidTom');
ylim([0 5]);
figure(5)
spectrogram(midtom,hamming(N),N/2,zp*N,Fs,'yaxis');
title('FM MidTom');
ylim([0 5]);
