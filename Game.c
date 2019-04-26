#include "Game.h"
#include "BSP.h"
#include "stdio.h"
/*********************************************** Global Vars *********************************************************************/
GameState_t state;
char readTouchName[] = "readTouch";
char idlethreadName[] = "idle";
char sendDatatName[] = "sendData";
char receiveDataName[] = "receiveData";
char drawName[] = "Draw";

/*********************************************** Common Threads *********************************************************************/

void Send(){
  while(1){
    G8RTOS_WaitSemaphore(&globalState);
    if(state.stackPos > state.lastSentStroke){
      for(; state.stackPos > state.lastSentStroke; state.lastSentStroke++){
        SendStroke(&state.selfQueue[state.lastSentStroke]);
        G8RTOS_SignalSemaphore(&globalState);
        sleep(4);
        G8RTOS_WaitSemaphore(&globalState);
      }
    }else{
      G8RTOS_SignalSemaphore(&globalState);
    }
    sleep(10);
  }
}

void Receive(){
  while(1){
    G8RTOS_WaitSemaphore(&globalState);
    ReceiveStroke(&state.selfQueue[state.friendStackPos]);
    state.friendStackPos++;
    G8RTOS_SignalSemaphore(&globalState);
    sleep(2);
  }
}

uint16_t touchX;
uint16_t touchY;
void ProcessTouch(){
  uint16_t lastX;
  uint16_t lastY;
  while(1){
    G8RTOS_WaitSemaphore(&globalState);
    if(touchX > DRAWABLE_MIN_X && touchX < DRAWABLE_MAX_X &&
       touchY > DRAWABLE_MIN_Y && touchY < DRAWABLE_MIN_Y){
      if(abs(touchX-lastX) > MIN_DIST || abs(touchX-lastY) > MIN_DIST){
        lastX = touchX;
        lastY = touchY;
        state.selfQueue[stackPos].pos.x = touchX - DRAWABLE_MIN_X;
        state.selfQueue[stackPos].pos.y = touchY - DRAWABLE_MIN_Y;
        state.selfQueue[stackPos].brush.color = state.currentBrush.color;
        state.selfQueue[stackPos].brush.size = state.currentBrush.size;
        stackPos++;
      }
    }
    G8RTOS_SignalSemaphore(&globalState);
    sleep(4);
  }
}

void ReadTouch(){
  //READ Touch
}

void Draw(){
  uint8_t prevBoard = -1;
  ScreenPos_t lastStackPosition = 0;
  while(1){
    if(prevBoard == state.currentBoard && state.currentBoard == SELF){
      if(state.stackPos > lastStackPosition){
        for(; state.statePos > lastStackPosition; lastStackPosition++){
          DrawStroke(&state.selfQueue[lastStackPosition]);
        }
      }
    }else if(prevBoard != state.currentBoard){
      DrawBoard();
      RedrawStrokes();
      prevBoard = state.currentBoard;
    }
    sleep(1);
  }
}

/*
 * Idle thread
 */
void IdleThread(){
  while(1){
      G8RTOS_Yield();
  }
}

 bool isHost = false;
 bool isClient = false;

 inline void initInterrupts(){
    //Button Init 4.4 (button 0) -> server/host
    P4->DIR &= ~BIT4;   //set as input
    P4->IFG &= ~BIT4;   //P4.4 IFG cleared
    P4->IE |= BIT4;     //Enable interrupt on P4.4
    P4->IES |= BIT4;    //falling edge triggered
    P4->REN |= BIT4;    //pull-up resistor
    P4->OUT |= BIT4;    //Sets res to pull-up

    //Button Init 5.4 (button 2) -> client
    P5->DIR &= ~BIT4;   //set as input
    P5->IFG &= ~BIT4;   //P5.4 IFG cleared
    P5->IE |= BIT4;     //Enable interrupt on P5.4
    P5->IES |= BIT4;    //falling edge triggered
    P5->REN |= BIT4;    //pull-up resistor
    P5->OUT |= BIT4;    //Sets res to pull-up

    NVIC_EnableIRQ(PORT4_IRQn);
    NVIC_EnableIRQ(PORT5_IRQn);
 }

bool connected = false;
bool decreaseBrush = false;
bool increaseBrush = false;
void PORT4_IRQHandler(){
    if((P4->IFG & BIT4 == BIT4) && !connected){     //if top button (b0) pressed and not connected yet
        isHost = true;
        connected = true;
        P4->IFG &= ~BIT4;   //clr flag 4.4
    }
    else if((P4->IFG & BIT4 == BIT4) && connected){   //top button pressed and connected, then viewFriendScreen
        board.currentBoard = FRIEND;
        P4->IFG &= ~BIT4;   //clr flag 4.4
    }
    else if(P4->IFG & BIT5 == BIT5){    //right button pressed, increase brush size
        increaseBrush = true;
        P4->IFG &= ~BIT5;   //clr flag 4.5
    }
}

void PORT5_IRQHandler(){
    if((P5->IFG & BIT4 == BIT4) && !connected){     //if bottom button (b2) pressed and not connected yet
        isClient = true;
        connected = true;
        P5->IFG &= ~BIT4;   //clr flag 5.4
    }
    else if((P5->IFG & BIT4 == BIT4) && connected){   //bottom button pressed and connected, then viewOwnScreen
        board.currentBoard = SELF;
        P5->IFG &= ~BIT4;   //clr flag 5.4
    }
    else if(P5->IFG & BIT5 == BIT5){    //left button pressed, decrease brush size
        decreaseBrush = true;
        P5->IFG &= ~BIT5;   //clr flag 5.5
    }
}

void WaitInit(){
    initInterrupts();
    while(!(isHost || isClient));
    G8RTOS_InitSemaphore(&cc3100, 1);
    G8RTOS_InitSemaphore(&lcd, 1);
    G8RTOS_InitSemaphore(&screenSelf, 1);
    G8RTOS_InitSemaphore(&screenFriend, 1);
    G8RTOS_InitSemaphore(&globalState, 1);
    if(isHost){
        InitHost();
    }else if(isClient){
        InitClient();
    }
    CreateThreadsAndStart();
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

void InitHost(){
  initCC3100(Host);
  while(ReceiveData((uint8_t * )&state.ip, sizeof(uint32_t)) == NOTHING_RECEIVED);

  uint8_t temp;
  SendData((uint8_t * ) &temp, state.ip, sizeof(uint8_t));

  P2->DIR |= RED_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 0) = 1;  //toggle led on
}

void InitClient(){
  initCC3100(Client);
  state.ip = getLocalIP();

  SendData((uint8_t *)&state.ip, HOST_IP_ADDR, sizeof(uint32_t));

  uint8_t temp;
  while(ReceiveData(&temp, sizeof(uint8_t)) == NOTHING_RECEIVED);

  P2->DIR |= BLUE_LED;  //p1.0 set to output
  BITBAND_PERI(P2->OUT, 2) = 1;  //toggle led on
}

void CreateThreadsAndStart(){
  while(1);
}

void DrawBoard(){
  G8RTOS_WaitSemaphore(&lcd);
  
  G8RTOS_SignalSemaphore(&lcd);
}

void RedrawStrokes(){

}

void DrawStroke(BrushStroke_t * stroke){

}

//Already has globalState Semaphore when called.
void SendStroke(BrushStroke_t * stroke){

}

//Already has globalState Semaphore when called.
void ReceiveStroke(BrushStroke_t * stroke){

}

inline void abs(){

}

/*********************************************** Public Functions *********************************************************************/
