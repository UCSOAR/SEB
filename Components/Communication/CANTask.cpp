/**
 ********************************************************************************
 * @file    ${CANTask.cpp}
 * @author  ${Alina}
 * @date    ${Oct 24, 2026}
 * @brief   This is a source file that initializes a CANBusTask that can switch the
 * 			SEB to be either a mother or daughter through UART.
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
extern FDCAN_HandleTypeDef hfdcan2;
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
enum SEBSetting {
	SEB_MOTHER,
	SEB_DAUGHTER
};
SEBSetting currentMode = SEB_DAUGHTER;

/**
 * @brief Initialize the CANBusTask
 *        Do not modify this function aside from adding the task name
 */
void CANBusTask::SwitchToDaughter() {
	uint8_t BOARD_TYPE = CAN_DEV_SEB;
	    	uint8_t SLOT_NUMBER = 123;

	    	dau = new CanAutoNodeDaughter{&hfdcan2,
	    		SOL_Init,
	    		SOL_COUNT,
				BOARD_TYPE,
				SLOT_NUMBER,
				CAN_GSE_TARGET_PT1
	    	};
	    	dau->TryRequestingJoiningNetwork();
	    	currentMode = SEB_DAUGHTER;
}
void CANBusTask::InitTask()
{
    // Make sure the task is not already initialized
    SOAR_ASSERT(rtTaskHandle == nullptr, "Cannot initialize CAN task twice");

    BaseType_t rtValue =
        xTaskCreate((TaskFunction_t)CANBusTask::RunTask,
            (const char*)"CANBusTask",
            (uint16_t){TASK_CAN_STACK_DEPTH_WORDS},
            (void*)this,
            (UBaseType_t){TASK_CAN_PRIORITY},
            (TaskHandle_t*)&rtTaskHandle);

            SOAR_ASSERT(rtValue == pdPASS, "CANBusTask::InitTask() - xTaskCreate() failed");


            SwitchToDaughter();
}

/**
 * @brief Instance Run loop for the Task, runs on scheduler start as long as the task is initialized.
 * @param pvParams RTOS Passed void parameters, contains a pointer to the object instance, should not be used
 */
void CANBusTask::Run(void * pvParams)
{

    while (1) {
    	if (currentMode == SEB_DAUGHTER) {
    		dau->CheckCANCommands();
			if(dau->GetCurrentState() == dau->READY) {
				RecieveData();
			} else {
				HAL_Delay(1000);
				dau->TryRequestingJoiningNetwork();
			}
    	}
    	if (currentMode == SEB_MOTHER) {
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

    }
}

/**
 * @brief Handles a command
 * @param cm Command reference to handle
 */
void CANBusTask::HandleCommand(Command& cm)
{
    switch (cm.GetCommand()) {

    case SEB_DAUGHTER: {
    	if (currentMode == SEB_MOTHER) {
    		if (mother != nullptr) {
    			delete mother;
    			mother = nullptr;
    		}
    		else {
    			break;
    		}
    	}

    	SwitchToDaughter();
    }

    case SEB_MOTHER: {
    	if (currentMode == SEB_DAUGHTER) {
    	   if (dau != nullptr) {
    	    	delete dau;
    	    	dau = nullptr;
    	    }
    	   else {
    		   break;
    	   }
    	}
    	mother = new CanAutoNodeMotherboard{&hfdcan2};
    	currentMode = SEB_MOTHER;
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
