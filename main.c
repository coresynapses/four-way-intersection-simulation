#include "stm32c0xx_hal.h"
#include <stdio.h>
#include <stdlib.h>

// Assuming a simple two-way intersection (North-South and East-West)
#define NUM_APPROACHES 2

// Signal timings (in milliseconds)
#define GREEN_TIME_MS   20
#define YELLOW_TIME_MS  6
#define CYCLE_LENGTH_MS (2 * (GREEN_TIME_MS + YELLOW_TIME_MS))

// Arrival rates (Cars per millisecond)
#define ARRIVAL_RATE_NS 0.0001 // 1 car every 10 seconds (10000 ms)
#define ARRIVAL_RATE_EW 0.00005 // 1 car every 20 seconds (20000 ms)

// GPIO pins for LEDs
#define NS_GREEN_PIN  GPIO_PIN_5
#define NS_YELLOW_PIN GPIO_PIN_6
#define NS_RED_PIN    GPIO_PIN_7

#define EW_GREEN_PIN  GPIO_PIN_0
#define EW_YELLOW_PIN GPIO_PIN_1
#define EW_RED_PIN    GPIO_PIN_4

#define LED_GPIO_PORT GPIOA


enum TrafficLight {
  Green,
  Yellow,
  Red
};

enum TrafficDirection {
  NorthSouth,
  EastWest,
  All
};

typedef struct {
    volatile uint32_t count; // Use volatile for shared access in potential interrupt contexts
} Queue;

void initializeQueue(Queue *q) {
    q->count = 0;
}

void enqueue(Queue *q) {
    q->count++;
}

uint32_t dequeueAll(Queue *q) {
    uint32_t cleared = q->count;
    q->count = 0;
    return cleared;
}

// Function to initialize GPIO pins for LEDs
void LED_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure GPIO pin Output Level
    HAL_GPIO_WritePin(LED_GPIO_PORT, NS_GREEN_PIN | NS_YELLOW_PIN | NS_RED_PIN |
                                     EW_GREEN_PIN | EW_YELLOW_PIN | EW_RED_PIN, GPIO_PIN_RESET);

    // Configure GPIO pins as output push-pull
    GPIO_InitStruct.Pin = NS_GREEN_PIN | NS_YELLOW_PIN | NS_RED_PIN |
                           EW_GREEN_PIN | EW_YELLOW_PIN | EW_RED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
}

// Function to control the LEDs
void setLED(uint16_t pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(LED_GPIO_PORT, pin, state);
}

void resetAll(enum TrafficDirection dir) {
  switch(dir) {
  case NorthSouth:
    setLED(NS_GREEN_PIN,  GPIO_PIN_RESET);
    setLED(NS_YELLOW_PIN, GPIO_PIN_RESET);
    setLED(NS_RED_PIN,    GPIO_PIN_RESET);
    return;

  case EastWest:
    setLED(EW_GREEN_PIN,  GPIO_PIN_RESET);
    setLED(EW_YELLOW_PIN, GPIO_PIN_RESET);
    setLED(EW_RED_PIN,    GPIO_PIN_RESET);
    return;

  case All:
    setLED(NS_GREEN_PIN,  GPIO_PIN_RESET);
    setLED(NS_YELLOW_PIN, GPIO_PIN_RESET);
    setLED(NS_RED_PIN,    GPIO_PIN_RESET);
    setLED(EW_GREEN_PIN,  GPIO_PIN_RESET);
    setLED(EW_YELLOW_PIN, GPIO_PIN_RESET);
    setLED(EW_RED_PIN,    GPIO_PIN_RESET);
    return;

  default:
    return;
  } 
}

void setLight(enum TrafficDirection dir, enum TrafficLight light) {
  resetAll(dir);

  switch(dir) {
  case NorthSouth:
    switch(light) {
    case Red:
      setLED(NS_RED_PIN, GPIO_PIN_SET);
      return;
    case Yellow:
      setLED(NS_YELLOW_PIN, GPIO_PIN_SET);
      return;
    case Green:
      setLED(NS_GREEN_PIN, GPIO_PIN_SET);
      return;
    default:
      return;
    }
    break;

  case EastWest:
    switch(light) {
    case Red:
      setLED(EW_RED_PIN, GPIO_PIN_SET);
      return;
    case Yellow:
      setLED(EW_YELLOW_PIN, GPIO_PIN_SET);
      return;
    case Green:
      setLED(EW_GREEN_PIN, GPIO_PIN_SET);
      return;
    default:
      return;
    }
    break;

  default:
    return;
  }
}

// System Clock Configuration
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure the main internal regulator output voltage
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
}

int main(void) {
    // Initialize the System
    HAL_Init();
    SystemClock_Config();
    LED_GPIO_Init();

    // Create the intersection
    Queue queues[NUM_APPROACHES];
    initializeQueue(&queues[0]); // North-South queue
    initializeQueue(&queues[1]); // East-West queue

    // Initialize variables
    uint32_t time = 0;
    int randomNum = 0;

    printf("Four-way Intersection Traffic Simulation\n");

    while (1) {
        // Car Arrivals
        randomNum = random() % 10;
        if (randomNum == 5) enqueue(&queues[0]); // North-South arrival
        if (randomNum == 4) enqueue(&queues[1]); // East-West arrival

        printf("Time: %lu ms, NS Queue: %lu, EW Queue: %lu, ", time, queues[0].count, queues[1].count);

        // Signal Logic and LED Control
        uint32_t time_in_cycle = time % CYCLE_LENGTH_MS;

        // resetAll(All);

        if (time_in_cycle < GREEN_TIME_MS) {
            printf("Signal: NS Green, Cleared NS: %lu\n", dequeueAll(&queues[0]));

            setLight(NorthSouth, Green);
            setLight(EastWest,   Red);

        } else if (time_in_cycle < GREEN_TIME_MS + YELLOW_TIME_MS) {
            printf("Signal: NS Yellow\n");

            setLight(NorthSouth, Yellow);
            setLight(EastWest,   Red);

        } else if (time_in_cycle < 2 * GREEN_TIME_MS + YELLOW_TIME_MS) {
            printf("Signal: EW Green, Cleared EW: %lu\n", dequeueAll(&queues[1]));

            setLight(NorthSouth, Red);
            setLight(EastWest,   Green);

        } else {
            printf("Signal: EW Yellow\n");

            // setLight(NorthSouth, Red);
            setLight(EastWest,   Yellow);
        }

         // Small delay for time progression
        HAL_Delay(1);
        time++;
    }
}
