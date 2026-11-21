#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Adafruit_ST7735_Macros.h"
#include "stm32h5xx.h"
#include "Adafruit_ST7735_API.h"
#include "Adafruit_TFTShield18_API.h"
#include "stm32h5xx_hal_rng.h"
#include "game.h"

typedef struct
{
    uint16_t freq;
    uint8_t button;
} FreqBtn;

typedef struct
{
    int8_t fb_index;
    uint8_t octave;
    uint16_t length;
} Note;

#define NUM_FB_PAIRS 28
#define FREQ_INCREMENT 35
#define LOWEST_FREQ 20

#define NUM_BUTTONS 4

#define GREEN 0
#define BLUE 1
#define RED 2
#define YELLOW 3
#define QUIT 4
#define NO_BUTTON 5

#define GREEN_FLAG 1
#define BLUE_FLAG (1 << 1)
#define RED_FLAG (1 << 2)
#define YELLOW_FLAG (1 << 3)
#define JOYSTICK_FLAG (1 << 4)

// leds are for testing
#define BLUE_LED_PORT GPIOC
#define BLUE_LED_MODE GPIO_MODER_MODE10
#define BLUE_LED_PUPD GPIO_PUPDR_PUPD10
#define BLUE_LED_MODE_0 GPIO_MODER_MODE10_0
#define BLUE_LED_MODE_1 GPIO_MODER_MODE10_1
#define BLUE_LED_PUPD_0 GPIO_PUPDR_PUPD10_0
#define BLUE_LED_PUPD_1 GPIO_PUPDR_PUPD10_1
#define BLUE_LED_OUTPUT GPIO_ODR_OD10

#define GREEN_LED_PORT GPIOC
#define GREEN_LED_MODE GPIO_MODER_MODE11
#define GREEN_LED_PUPD GPIO_PUPDR_PUPD11
#define GREEN_LED_MODE_0 GPIO_MODER_MODE11_0
#define GREEN_LED_MODE_1 GPIO_MODER_MODE11_1
#define GREEN_LED_PUPD_0 GPIO_PUPDR_PUPD11_0
#define GREEN_LED_PUPD_1 GPIO_PUPDR_PUPD11_1
#define GREEN_LED_OUTPUT GPIO_ODR_OD11

#define RED_LED_PORT GPIOC
#define RED_LED_MODE GPIO_MODER_MODE12
#define RED_LED_PUPD GPIO_PUPDR_PUPD12
#define RED_LED_MODE_0 GPIO_MODER_MODE12_0
#define RED_LED_MODE_1 GPIO_MODER_MODE12_1
#define RED_LED_PUPD_0 GPIO_PUPDR_PUPD12_0
#define RED_LED_PUPD_1 GPIO_PUPDR_PUPD12_1
#define RED_LED_OUTPUT GPIO_ODR_OD12

#define YELLOW_LED_PORT GPIOD
#define YELLOW_LED_MODE GPIO_MODER_MODE2
#define YELLOW_LED_PUPD GPIO_PUPDR_PUPD2
#define YELLOW_LED_MODE_0 GPIO_MODER_MODE2_0
#define YELLOW_LED_MODE_1 GPIO_MODER_MODE2_1
#define YELLOW_LED_PUPD_0 GPIO_PUPDR_PUPD2_0
#define YELLOW_LED_PUPD_1 GPIO_PUPDR_PUPD2_1
#define YELLOW_LED_OUTPUT GPIO_ODR_OD2

#define GREEN_BUTTON_PORT GPIOA
#define GREEN_BUTTON_MODE GPIO_MODER_MODE6
#define GREEN_BUTTON_PUPD GPIO_PUPDR_PUPD6
#define GREEN_BUTTON_MODE_0 GPIO_MODER_MODE6_0
#define GREEN_BUTTON_MODE_1 GPIO_MODER_MODE6_1
#define GREEN_BUTTON_PUPD_0 GPIO_PUPDR_PUPD6_0
#define GREEN_BUTTON_PUPD_1 GPIO_PUPDR_PUPD6_1
#define GREEN_BUTTON_INPUT GPIO_IDR_ID6

#define BLUE_BUTTON_PORT GPIOA
#define BLUE_BUTTON_MODE GPIO_MODER_MODE7
#define BLUE_BUTTON_PUPD GPIO_PUPDR_PUPD7
#define BLUE_BUTTON_MODE_0 GPIO_MODER_MODE7_0
#define BLUE_BUTTON_MODE_1 GPIO_MODER_MODE7_1
#define BLUE_BUTTON_PUPD_0 GPIO_PUPDR_PUPD7_0
#define BLUE_BUTTON_PUPD_1 GPIO_PUPDR_PUPD7_1
#define BLUE_BUTTON_INPUT GPIO_IDR_ID7

#define RED_BUTTON_PORT GPIOB
#define RED_BUTTON_MODE GPIO_MODER_MODE10
#define RED_BUTTON_PUPD GPIO_PUPDR_PUPD10
#define RED_BUTTON_MODE_0 GPIO_MODER_MODE10_0
#define RED_BUTTON_MODE_1 GPIO_MODER_MODE10_1
#define RED_BUTTON_PUPD_0 GPIO_PUPDR_PUPD10_0
#define RED_BUTTON_PUPD_1 GPIO_PUPDR_PUPD10_1
#define RED_BUTTON_INPUT GPIO_IDR_ID10

#define YELLOW_BUTTON_PORT GPIOC
#define YELLOW_BUTTON_MODE GPIO_MODER_MODE6
#define YELLOW_BUTTON_PUPD GPIO_PUPDR_PUPD6
#define YELLOW_BUTTON_MODE_0 GPIO_MODER_MODE6_0
#define YELLOW_BUTTON_MODE_1 GPIO_MODER_MODE6_1
#define YELLOW_BUTTON_PUPD_0 GPIO_PUPDR_PUPD6_0
#define YELLOW_BUTTON_PUPD_1 GPIO_PUPDR_PUPD6_1
#define YELLOW_BUTTON_INPUT GPIO_IDR_ID6

#define TIMER_PSC 31
#define TIMER_ARR 999

// all of these are in ms
#define TIMER_PERIOD 1
#define SHORTEST_NOTE 100

#define MS_PER_PIXEL 25
#define MS_PER_FRAME 10
#define GRACE_PERIOD 500

FreqBtn fb_pairs[NUM_FB_PAIRS];
volatile uint32_t game_clock;
volatile uint16_t this_note_clock;
Note* song;
volatile uint32_t song_index;
uint32_t song_size;
uint32_t song_time; // how long the song is in ms
volatile uint32_t score;

#define SONG_NUM_NOTES 20
#define NUM_PITCHES 4
#define NOTE_MIN_LENGTH 100
#define NOTE_MAX_LENGTH 1000

uint32_t PARTITION = 0;

bool done = false;

void set_led(int led, bool on)
{
    switch (led)
    {
        case GREEN:
        {
            if (on)
            {
                GREEN_LED_PORT->ODR |= GREEN_LED_OUTPUT;
            }
            else
            {
                GREEN_LED_PORT->ODR &= ~GREEN_LED_OUTPUT;
            }
            break;
        }
        case BLUE:
        {
            if (on)
            {
                BLUE_LED_PORT->ODR |= BLUE_LED_OUTPUT;
            }
            else
            {
                BLUE_LED_PORT->ODR &= ~BLUE_LED_OUTPUT;
            }
            break;
        }
        case RED:
        {
            if (on)
            {
                RED_LED_PORT->ODR |= RED_LED_OUTPUT;
            }
            else
            {
                RED_LED_PORT->ODR &= ~RED_LED_OUTPUT;
            }
            break;
        }
        case YELLOW:
        {
            if (on)
            {
                YELLOW_LED_PORT->ODR |= YELLOW_LED_OUTPUT;
            }
            else
            {
                YELLOW_LED_PORT->ODR &= ~YELLOW_LED_OUTPUT;
            }
            break;
        }
    }
}

void start_timer(void)
{
    // enable TIM2's clock (|= : other APB1 peripherals are already on)
    RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;

    TIM2->PSC  = TIMER_PSC;
    TIM2->ARR  = TIMER_ARR;
    TIM2->EGR  = TIM_EGR_UG;      // load PSC now (also sets UIF:
                                  // the first interrupt arrives right away)
    TIM2->DIER = TIM_DIER_UIE;    // update interrupt enabled
    NVIC_EnableIRQ(TIM2_IRQn);    // TIM2's interrupt line enabled in the NVIC
    TIM2->CR1  = TIM_CR1_CEN;     // start counting
}

void stop_timer(void)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;    // stop counting
    NVIC_DisableIRQ(TIM2_IRQn);
}

void display_score(void)
{
    Display_fillScreen(ST7735_BLACK);
    Display_setCursor(0, 0);
    double final_score = ((double) score / (double) song_time) * 100;
    char str[5];
    str[0] = '0' + (int) (final_score / 10) % 10;
    str[1] = '0' + (int) final_score % 10;
    str[2] = '.';
    str[3] = '0' + (int) (final_score * 10) % 10;
    str[4] = '\0';
    Display_print(str);
}

uint8_t get_input(bool check_for_joystick)
{
    uint8_t r = 0;
    if (GREEN_BUTTON_PORT->IDR & GREEN_BUTTON_INPUT)
    {
        r |= GREEN_FLAG;
    }
    if (BLUE_BUTTON_PORT->IDR & BLUE_BUTTON_INPUT)
    {
        r |= BLUE_FLAG;
    }
    if (RED_BUTTON_PORT->IDR & RED_BUTTON_INPUT)
    {
        r |= RED_FLAG;
    }
    if (YELLOW_BUTTON_PORT->IDR & YELLOW_BUTTON_INPUT)
    {
        r |= YELLOW_FLAG;
    }
    if (check_for_joystick && !(TFTShield18_readButtons() & TFTSHIELD_BUTTON_IN))
    {
        r |= JOYSTICK_FLAG;
    }
    return r;
}

uint8_t get_expected_input(int button)
{
    switch (button)
    {
        case GREEN:
        {
            return GREEN_FLAG;
            break;
        }
        case BLUE:
        {
            return BLUE_FLAG;
            break;
        }
        case RED:
        {
            return RED_FLAG;
            break;
        }
        case YELLOW:
        {
            return YELLOW_FLAG;
            break;
        }
    }

    return 0;
}

// The name must match the vector table in startup_stm32h533xx.s exactly.
void TIM2_IRQHandler(void)
{
    int fb_index = song[song_index].fb_index;
    if (fb_index >= 0)
    {
        int val = get_input(false);
        val ^= get_expected_input(fb_pairs[fb_index].button);
        if (!val)
        {
            score++;
        }
    }

    game_clock++;
    this_note_clock++;

    if (this_note_clock >= song[song_index].length)
    {
        this_note_clock = 0;
        song_index++;
    }
    if (game_clock >= song_time || song_index >= song_size)
    {
        stop_timer();
        done = true;
    }

    TIM2->SR &= ~TIM_SR_UIF;       // clear the update flag: write 0, don't modify
}

bool game_init(RNG_HandleTypeDef* hrng)
{
    for (int i = 0; i < NUM_FB_PAIRS; i++)
    {
        fb_pairs[i].freq = LOWEST_FREQ + i * FREQ_INCREMENT;
        fb_pairs[i].button = i % NUM_BUTTONS;
    }

    BLUE_LED_PORT->MODER &= ~BLUE_LED_MODE;
    BLUE_LED_PORT->MODER |= BLUE_LED_MODE_0;
    GREEN_LED_PORT->MODER &= ~GREEN_LED_MODE;
    GREEN_LED_PORT->MODER |= GREEN_LED_MODE_0;
    RED_LED_PORT->MODER &= ~RED_LED_MODE;
    RED_LED_PORT->MODER |= RED_LED_MODE_0;
    YELLOW_LED_PORT->MODER &= ~YELLOW_LED_MODE;
    YELLOW_LED_PORT->MODER |= YELLOW_LED_MODE_0;

    BLUE_BUTTON_PORT->MODER &= ~BLUE_BUTTON_MODE;
    BLUE_BUTTON_PORT->PUPDR &= ~BLUE_BUTTON_PUPD;
    BLUE_BUTTON_PORT->PUPDR |= BLUE_BUTTON_PUPD_1;
    GREEN_BUTTON_PORT->MODER &= ~GREEN_BUTTON_MODE;
    GREEN_BUTTON_PORT->PUPDR &= ~GREEN_BUTTON_PUPD;
    GREEN_BUTTON_PORT->PUPDR |= GREEN_BUTTON_PUPD_1;
    RED_BUTTON_PORT->MODER &= ~RED_BUTTON_MODE;
    RED_BUTTON_PORT->PUPDR &= ~RED_BUTTON_PUPD;
    RED_BUTTON_PORT->PUPDR |= RED_BUTTON_PUPD_1;
    YELLOW_BUTTON_PORT->MODER &= ~YELLOW_BUTTON_MODE;
    YELLOW_BUTTON_PORT->PUPDR &= ~YELLOW_BUTTON_PUPD;
    YELLOW_BUTTON_PORT->PUPDR |= YELLOW_BUTTON_PUPD_1;

    score = 0;
    game_clock = 0;
    this_note_clock = 0;
    song_index = 0;

    PARTITION = Display_width() / 4;

    song = malloc(sizeof(Note) * SONG_NUM_NOTES + 1);
    song_size = SONG_NUM_NOTES + 1;
    song_time = GRACE_PERIOD;

    song[0].fb_index = -1;
    song[0].octave = 0;
    song[0].length = GRACE_PERIOD;

    for (int i = 1; i < SONG_NUM_NOTES + 1; i++)
    {
        uint32_t rand;

        HAL_StatusTypeDef status = HAL_RNG_GenerateRandomNumber(hrng, &rand);
        if (status != HAL_OK)
        {
            free(song);
            return false;
        }
        song[i].fb_index = rand % NUM_FB_PAIRS;

        status = HAL_RNG_GenerateRandomNumber(hrng, &rand);
        if (status != HAL_OK)
        {
            free(song);
            return false;
        }
        song[i].octave = rand % NUM_PITCHES + 1;

        status = HAL_RNG_GenerateRandomNumber(hrng, &rand);
        if (status != HAL_OK)
        {
            free(song);
            return false;
        }
        song[i].length = rand % (NOTE_MAX_LENGTH - NOTE_MIN_LENGTH) + NOTE_MIN_LENGTH;
        song_time += song[i].length;
    }

    start_timer();

    return true;
}

void print_incoming_color(uint8_t color, uint32_t note_clock)
{
    int y = Display_height() - PARTITION;
    int next_occurrence = song_index;
    int time_until_occurrence = -1 * note_clock;
    while (1)
    {
        for (; song[next_occurrence].fb_index < 0 && next_occurrence < song_size; next_occurrence++)
        {
            time_until_occurrence += song[next_occurrence].length;
        }
        for (; fb_pairs[song[next_occurrence].fb_index].button != color && next_occurrence < song_size; next_occurrence++)
        {
            time_until_occurrence += song[next_occurrence].length;
        }

        if (next_occurrence == song_size || time_until_occurrence / MS_PER_PIXEL >= y)
        {
            Display_fillRect(PARTITION * color, 0, PARTITION, y, ST7735_BLACK);
            break;
        }
        else if (time_until_occurrence < 0)
        {
            int time_left = song[next_occurrence].length + time_until_occurrence;
            int pixels = time_left / MS_PER_PIXEL;
            if (pixels > y)
            {
                pixels = y;
            }
            Display_fillRect(PARTITION * color, y - pixels, PARTITION, pixels, ST7735_WHITE);
            y -= pixels;
            if (y <= 0)
            {
                break;
            }
            time_until_occurrence = 0;
            next_occurrence++;
        }
        else
        {
            int pixels = time_until_occurrence / MS_PER_PIXEL;
            if (pixels > y)
            {
                pixels = y;
            }
            Display_fillRect(PARTITION * color, y - pixels, PARTITION, pixels, ST7735_BLACK);
            y -= pixels;
            if (y <= 0)
            {
                break;
            }
            pixels = song[next_occurrence].length / MS_PER_PIXEL;
            if (pixels > y)
            {
                pixels = y;
            }
            Display_fillRect(PARTITION * color, y - pixels, PARTITION, pixels, ST7735_WHITE);
            y -= pixels;
            if (y <= 0)
            {
                break;
            }
            time_until_occurrence = 0;
            next_occurrence++;
        }
    }
}

void do_display(void)
{
    static uint8_t last_input = 0xFF;
    static uint32_t last_game_clock = 0;
//    static uint8_t last_expected = 0xFF;
    if (done)
    {
        display_score();
        while (1) {}
    }
    set_led(BLUE, true);
    uint8_t input = get_input(true);
    if (input & JOYSTICK_FLAG)
    {

    }
    set_led(BLUE, false);
    if (input != last_input)
    {
        if (input & GREEN_FLAG)
        {
            Display_fillRect(PARTITION * GREEN, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_GREEN);
        }
        else
        {
            Display_fillRect(PARTITION * GREEN, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_WHITE);
        }
        if (input & BLUE_FLAG)
        {
            Display_fillRect(PARTITION * BLUE, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_BLUE);
        }
        else
        {
            Display_fillRect(PARTITION * BLUE, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_WHITE);

        }
        if (input & RED_FLAG)
        {
            Display_fillRect(PARTITION * RED, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_RED);
        }
        else
        {
            Display_fillRect(PARTITION * RED, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_WHITE);

        }
        if (input & YELLOW_FLAG)
        {
            Display_fillRect(PARTITION * YELLOW, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_YELLOW);
        }
        else
        {
            Display_fillRect(PARTITION * YELLOW, Display_height() - PARTITION, PARTITION, PARTITION, ST7735_WHITE);
        }
        last_input = input;
    }

    if (game_clock / MS_PER_FRAME != last_game_clock / MS_PER_FRAME)
    {
        for (int i = 0; i < NUM_BUTTONS; i++)
        {
            print_incoming_color(i, this_note_clock);
        }
    }
    last_game_clock = game_clock;
}

void do_buzzer(void)
{

}

void game_execute(void)
{
    do_display();
    do_buzzer();
}