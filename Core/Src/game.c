#include <stdint.h>
#include "stm32h5xx.h"
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
#define SELECT_FLAG (1 << 4)

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

FreqBtn fb_pairs[NUM_FB_PAIRS];
uint32_t game_clock;
uint16_t this_note_clock;
Note* song;
uint32_t song_index;
uint32_t song_size;
uint32_t song_time; // how long the song is in ms
uint32_t score;

const uint32_t PARTITION = Display_width() / 4;

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

uint8_t get_input(void)
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
    if (!(TFTShield18_readButtons() & TFTSHIELD_BUTTON_IN))
    {
        r |= SELECT_FLAG;
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
    TIM2->SR = ~TIM_SR_UIF;       // clear the update flag: write 0, don't modify

    if (!(get_input() ^ get_expected_input(fb_pairs[song[song_index].fb_index].button)))
    {
        score++;
    }

    game_clock++;
    this_note_clock++;

    if (this_note_clock >= song[song_index].length)
    {
        this_note_clock = 0;
        song_index++;
    }
}

void game_init(void)
{
    for (int i = 0; i < NUM_FB_PAIRS; i++)
    {
        fb_pairs[i].freq = LOWEST_FREQ + i * FREQ_INCREMENT;
        fb_pairs[i].button = i % NUM_BUTTONS;
    }

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
}

void do_display(void)
{
    static uint8_t last_input = 0xFF;
    static uint8_t last_expected = 0xFF;
    uint8_t input = get_input();
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

    uint8_t expected = get_expected_input(fb_pairs[song[song_index].fb_index].button);
    if (expected != last_expected)
    {
        if (expected & GREEN_FLAG)
        {
            Display_fillRect(PARTITION * GREEN, 0, PARTITION, Display_height() - PARTITION, ST7735_WHITE);
        }
        else
        {
            Display_fillRect(PARTITION * GREEN, 0, PARTITION, Display_height() - PARTITION, ST7735_BLACK);
        }
        if (expected & BLUE_FLAG)
        {
            Display_fillRect(PARTITION * BLUE, 0, PARTITION, Display_height() - PARTITION, ST7735_WHITE);
        }
        else
        {
            Display_fillRect(PARTITION * BLUE, 0, PARTITION, Display_height() - PARTITION, ST7735_BLACK);
        }
        if (expected & RED_FLAG)
        {
            Display_fillRect(PARTITION * RED, 0, PARTITION, Display_height() - PARTITION, ST7735_WHITE);
        }
        else
        {
            Display_fillRect(PARTITION * RED, 0, PARTITION, Display_height() - PARTITION, ST7735_BLACK);
        }
        if (expected & YELLOW_FLAG)
        {
            Display_fillRect(PARTITION * YELLOW, 0, PARTITION, Display_height() - PARTITION, ST7735_WHITE);
        }
        else
        {
            Display_fillRect(PARTITION * YELLOW, 0, PARTITION, Display_height() - PARTITION, ST7735_BLACK);
        }
        last_expected = expected;
    }
}

void do_buzzer(void)
{

}

void game_execute(void)
{
    do_display();
    do_buzzer();
}