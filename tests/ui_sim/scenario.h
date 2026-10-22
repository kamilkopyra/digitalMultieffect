// skrypt dla symulatora: sterowanie enkoderami (E1..E3 = indeksy 0..2), jak na sprzęcie
static void scenario()
{
	simPeak = 0.35f;
	wait(300);
	shot("01_start");                    // P1 Clean, fokus: Compressor

	enc(0, +8);                          // treshold 40 -> 48
	enc(2, -5);                          // makeupGain 55 -> 50
	push(0);                             // fokus: Equalizer
	enc(1, +6);                          // Mid
	shot("02_knobs");

	push(0);                             // fokus: Delay
	push(1);                             // Delay ON
	push(0);                             // fokus: master (VOL)
	simPeak = 1.0f;                      // przester na wejściu
	wait(60);
	simPeak = 0.55f;
	wait(90);
	shot("03_master");

	push(0);                             // fokus z powrotem na efekt 1
	push(2);                             // menu
	shot("04_menu");

	push(0);                             // Change effect
	enc(0, +4);                          // Compressor -> ... -> Flanger
	shot("05_picker");
	push(2);                             // wstecz bez zmiany

	push(2);                             // menu
	enc(0, +1);                          // Presets
	push(0);
	enc(0, +2);                          // P3
	push(1);                             // Save: pierwsze wciśnięcie = potwierdź
	shot("06_presets");
	wait(3200);                          // potwierdzenie wygasa
	push(0);                             // Load P3 (Lead)
	shot("07_lead");

	push(2);                             // menu
	enc(0, +1);                          // Tuner
	push(0);
	simTunerHz = 106.8f;                 // A2 za nisko
	wait(250);
	shot("08_tuner");
	simTunerHz = 110.1f;                 // A2 w stroju
	wait(250);
	shot("09_tuner_in_tune");
	simTunerHz = 0;
	push(2);                             // wstecz

	push(2);                             // menu
	enc(0, +1);                          // Settings
	push(0);
	enc(1, -12);                         // jasność 80 -> 68
	enc(0, +1);                          // Footswitch
	enc(1, +1);                          // -> Presets
	enc(0, +1);                          // Reset settings
	push(0);                             // pierwsze wciśnięcie
	wait(600);
	shot("10_settings");
	push(2);                             // wstecz (bez resetu)

	push(2);                             // menu
	enc(0, +1);                          // (Settings -> Change effect, kursor się zawija)
	push(0);
	enc(0, +6);                          // Rat (3) -> Phaser (9)
	push(0);                             // wybierz
	push(1);                             // Phaser ON
	push(2);                             // TAP
	wait(490);
	push(2);                             // TAP -> 120 BPM
	shot("11_phaser_tap");
}
