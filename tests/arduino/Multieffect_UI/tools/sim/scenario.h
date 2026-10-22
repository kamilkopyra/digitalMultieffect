// skrypt dotyku dla symulatora — współrzędne z makr szkicu
static int tileCX(int i) { return tileX0(i) + TILE_W / 2; }
#define TILE_CY ((TILE_Y0 + TILE_Y1) / 2)
#define BTN_CY  ((BTN_Y0 + BTN_Y1) / 2)
#define TOP_CY  ((TOP_Y0 + TOP_Y1) / 2)
#define BACK()  tap((BACK_X0 + BACK_X1) / 2, TOP_CY)

static void scenario()
{
  wait(700);
  tap(0, 319);                             // pusty dotyk: odśwież miernik
  shot("01_start");

  tap(tileCX(0), TILE_CY);                 // Compressor OFF (kafel w fokusie)
  tap(tileCX(1), TILE_CY);                 // fokus: Equalizer
  press(BAR_X0 + 10, BAR_Y(0) + 5);        // Low przeciągnięty do ~75
  press(BAR_X0 + (BAR_X1 - BAR_X0) * 3 / 4, BAR_Y(0) + 5);
  release();
  tap(tileCX(2), TILE_CY);                 // fokus: Delay
  tap(tileCX(2), TILE_CY);                 // Delay ON
  shot("02_edit_modified");

  tap((VOL_X0 + VOL_X1) / 2, TOP_CY);      // fokus: master
  wait(150);
  tap(0, 319);
  shot("03_master");

  tap((PRE_X0 + PRE_X1) / 2, TOP_CY);      // presety
  { int x0, y0; presetCell(2, &x0, &y0); tap(x0 + 20, y0 + 20); }   // zaznacz P3
  shot("04_presets");
  tap((BTN_L_X0 + BTN_L_X1) / 2, BTN_CY);  // Load P3 (Lead)
  tap(tileCX(1), TILE_CY);
  shot("05_lead");

  tap((TUN_X0 + TUN_X1) / 2, TOP_CY);      // tuner
  until([] { return tunerValid && abs(tunerCents) > 15 && shownState > 0; });
  shot("06_tuner");
  until([] { return shownState == 0; });
  shot("07_tuner_in_tune");
  BACK();

  tap((SET_X0 + SET_X1) / 2, TOP_CY);      // ustawienia
  tap(FS_MID + 40, ST_Y(1) + 15);          // footswitche: presety
  tap((BTN_L_X0 + BTN_L_X1) / 2, BTN_CY);  // reset: pierwsze dotknięcie
  shot("08_settings");
  BACK();

  tap((BTN_L_X0 + BTN_L_X1) / 2, BTN_CY);  // Change effect
  shot("09_picker");
}
