#include <NeoPixelBrightnessBus.h>

void updateAllLeds(NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> &strip, RgbColor color)
{
    for (int i = 0; i < strip.PixelCount(); i++)
    {
        strip.SetPixelColor(i, color);
    }
    strip.Show();
}

void updateAllLeds(NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> &strip, RgbColor color, uint8_t brightness)
{
    strip.SetBrightness(brightness);
    for (int i = 0; i < strip.PixelCount(); i++)
    {
        strip.SetPixelColor(i, color);
    }
    strip.Show();
}

void updateNthLed(NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> &strip, RgbColor color, uint8_t brightness, uint8_t n)
{
    strip.SetBrightness(brightness);
    strip.SetPixelColor(n, color);
    strip.Show();
}