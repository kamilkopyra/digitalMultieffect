#include "daisy_seed.h"
using namespace daisy;

static DaisySeed hw;
static Encoder   enc[3];

int main(void)
{
    hw.Init();
    hw.StartLog(true);

    enc[0].Init(seed::D6,  seed::D7,  seed::D8);   // A, B, SW  (Arduino 2, 3, 4)
    enc[1].Init(seed::D9,  seed::D10, seed::D11);  //           (Arduino 5, 6, 7)
    enc[2].Init(seed::D12, seed::D13, seed::D14);  //           (Arduino 8, 9, 10)

    hw.PrintLine("start");
    while(1)
    {
        for(int i = 0; i < 3; i++)
        {
            enc[i].Debounce();
            int d = enc[i].Increment();           
            if(d != 0)               hw.PrintLine("E %d %d", i, d);
            if(enc[i].RisingEdge())  hw.PrintLine("B %d", i);   
        }
        System::Delay(1);
    }
}