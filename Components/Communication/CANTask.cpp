/*
 * CANBusTask.cpp
 *
 *  Created on: Oct. 10, 2026
 *      Author: adam
 */
/**
 ********************************************************************************
 * @file    ${CANTask.cpp}
 * @author  ${Alina}
 * @date    ${Oct 24, 2026}
 * @brief   This is a template source file to create a new task in our firmware
 *
 * Setup Steps
 * 1. Define the Task Queue Depth in SystemDefines.hpp
 * 2. Define the Task Stack Depth in SystemDefines.hpp
 * 3. Define the Task Priority in SystemDefines.hpp
 * 4. Replace all placeholders marked with a $ sign
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "CANTask.hpp"
#include "SystemDefines.hpp"
#include "CANBusTargets.hpp"
#include "SOLLogs.hpp"



/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
extern FDCAN_HandleTypeDef hfdcan1;
/************************************
 * VARIABLES
 ************************************/

/************************************
 * FUNCTION DECLARATIONS
 ************************************/

/************************************
 * FUNCTION DEFINITIONS
 ************************************/

/**
 * @brief Constructor for CANBusTask
 */
CANBusTask::CANBusTask() : Task(TASK_CAN_QUEUE_DEPTH_OBJS)

{

}


bool isDaughter = false;
bool isMother = false;
/**
 * @brief Initialize the CANBusTask
 *        Do not modify this function aside from adding the task name
 */
void CANBusTask::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize watchdog task twice");

    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)CANBusTask::RunTask,
            (const char*)"CANBusTask",
            (uint16_t){TASK_CAN_STACK_DEPTH_WORDS},
            (void*)this,
            (UBaseType_t){TASK_CAN_PRIORITY},
            (TaskHandle_t*)&rtTaskHandle);

                SOAR_ASSERT(rtValue == pdPASS, "CANBusTask::InitTask() - xTaskCreate() failed");


	 uint8_t BOARD_TYPE = CAN_DEV_SEB;
	 uint8_t SLOT_NUMBER = 123; // was intended to be used at some point in development but decided not to.
	 	 	 	 	 	 	 	 // you can still use it as a way to identify boards.

    //init canbus these will have to be configured in the ioc.
	 // The SOL_INIT and SOL_COUNT macros are defined in the LogTypes folder in CommunicationSystemsSubmodule.
	 // In this example, they are for the FSB SOL board, but new ones for your board should be created there.
    dau = new CanAutoNodeDaughter{&hfdcan1,
    	SOL_Init,
		SOL_COUNT,
		BOARD_TYPE,
		SLOT_NUMBER,
		CAN_GSE_TARGET_PT1
    };
    //HAL_GPIO_WritePin(CAN_Standby_GPIO_Port, CAN_Standby_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Instance Run loop for the Task, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
 */
void CANBusTask::Run(void * pvParams)
{

    while (1) {
    	if (isDaughter) {
			if(dau->GetCurrentState() == dau->READY) {
				RecieveData();
			} else {
				HAL_Delay(1000);
				dau->TryRequestingJoiningNetwork();
			}
    	}
    	if (isMother) {
    		mother->CheckCANCommands();
    		// Receive heartbeat from daughter boards
    		if (mother->GetTicksSinceLastHeartbeat() > 500) {
    			mother->Heartbeat();
    		}
    	}
        /* Process commands in blocking mode */
        Command cm;
        bool res = qEvtQueue->Receive(cm);
        if(res) {
            HandleCommand(cm);
        }
        HAL_Delay(10);

        // How you might send a message, replacing the LOGID with a LOGID define from the LogTypes folder.
        //dau->SendMessageToMotherboardByLogID(LOGID, (uint8_t*)DATA_PTR);



    }
}

/**
 * @brief Handles a command
 * @param cm Command reference to handle
 */
void CANBusTask::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {

    case CANTASK_SWITCH_DAUGHTER: {
    	if (isMother == true) {
    		if (mother != nullptr) {
    			delete mother;
    		}
    	}

    	uint8_t BOARD_TYPE = CAN_DEV_SEB;
    	uint8_t SLOT_NUMBER = 123; // was intended to be used at some point in development but decided not to.
    		 	 	 	 	 	 	 	 // you can still use it as a way to identify boards.

    	    //init canbus these will have to be configured in the ioc.
    		 // The SOL_INIT and SOL_COUNT macros are defined in the LogTypes folder in CommunicationSystemsSubmodule.
    		 // In this example, they are for the FSB SOL board, but new ones for your board should be created there.
    	dau = new CanAutoNodeDaughter{&hfdcan1,
    		SOL_Init,
    		SOL_COUNT,
			BOARD_TYPE,
			SLOT_NUMBER,
			CAN_GSE_TARGET_PT1
    	};
    	dau->TryRequestingJoiningNetwork();
    	isDaughter = true;
    	isMother = false;
    }

    case CANTASK_SWITCH_MOTHER: {
    	if (isDaughter == true) {
    	   if (dau != nullptr) {
    	    	delete dau;
    	    }
    	}
    	mother = new CanAutoNodeMotherboard{&hfdcan1};
    	isMother = true;
    	isDaughter = false;
    }

    default:
        SOAR_PRINT("CANBusTask - Received Unsupported Command {%d}\n", cm.GetCommand());
        break;
    }

    //No matter what we happens, we must reset allocated data
    cm.Reset();
}


void CANBusTask::RecieveData(){
	dau->CheckCANCommands();

	// Example for how to handle an incoming message, taken from the RPB

//	bool foundone = false;
//		{
//			RPB_AIR_BRAKES_COMMAND cmd;
//			if(dau.ReadMessageByLogIndex(_RPB_AIR_BRAKES_COMMAND_LOGINDEX, (uint8_t*)&cmd, sizeof(cmd))) {
//				SOAR_PRINT("got airbrakes cmd %d\n",cmd.openAirBrakes);
//				if(cmd.openAirBrakes) {
//					AirbrakesTask::Inst().SendCommand({TASK_SPECIFIC_COMMAND,AIRBRAKES_COMMAND_OPEN});
//
//				} else {
//					AirbrakesTask::Inst().SendCommand({TASK_SPECIFIC_COMMAND,AIRBRAKES_COMMAND_CLOSE});
//				}
//			}
//			foundone= true;
//		}


}
