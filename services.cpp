/* VM management services aligned with slot-based scheduler */

#include "services.h"
#include "hypervisor_stubs.h"

#define false	0
#define true 	1
#define bool 	int

/**
 * @brief Starts execution of a VM selected by the scheduler
 *
 * This function is called after the scheduler selects a VM
 * that is in READY state. It updates the currently running VM
 * in the hypervisor control block and performs the low-level
 * context switch to transfer execution to that VM.
 *
 * @param vmId Virtual machine ID
 * @return true if the VM was started successfully, false otherwise
 */
int startVM(VM_Id_t vmId)
{
    VCoreType* vcore;

    if ((G_HVCB == NULL_PTR) || (vmId >= G_HVCB->no_of_Vcores))
    {
        return 0;
    }

    vcore = &G_HVCB->Vcore_list[vmId];

    /* Ensure VM is ready before execution */
    if (vcore->state != SCHED_VCORE_READY_STATE)
    {
        return 0;
    }

    /* Update currently running VM */
    G_HVCB->current_vm = vmId;

    /* Perform hardware-level context switch */
    HvTcStartVm_Mod(vmId);

    return 1;
}

/**
 * @brief Stops a virtual machine
 *
 * Marks the VM as STOPPED so it is no longer eligible for scheduling.
 * If the VM is currently running, the scheduler is triggered to
 * switch execution to another VM.
 *
 * @param vmId Virtual machine ID
 * @return true if VM stopped successfully, false otherwise
 */
int stopVM(VM_Id_t vmId)
{
    VCoreType* vcore;

    if ((G_HVCB == NULL_PTR) || (vmId >= G_HVCB->no_of_Vcores))
    {
        return 0;
    }

    vcore = &G_HVCB->Vcore_list[vmId];

    disableSchedulerInterrupts();

    /* Mark VM as STOPPED to prevent further scheduling */
    vcore->state = SCHED_VCORE_STOPPED_STATE;

    enableSchedulerInterrupts();

    /* If VM was running, trigger scheduler to select another VM */
    if (G_HVCB->current_vm == vmId)
    {
        Scheduler_AdvanceSlot(G_HVCB, G_SCHED_CB);
    }

    return 1;
}

/**
 * @brief Resets a virtual machine
 *
 * Stops the VM if it is running, reinitializes its execution
 * context (stack, registers, etc.), and sets it back to READY
 * state so it can be scheduled again.
 *
 * @param vmId Virtual machine ID
 * @return true if VM reset successfully, false otherwise
 */
int resetVM(VM_Id_t vmId)
{
    VCoreType* vcore;

    if ((G_HVCB == NULL_PTR) || (vmId >= G_HVCB->no_of_Vcores))
    {
        return 0;
    }

    vcore = &G_HVCB->Vcore_list[vmId];

    disableSchedulerInterrupts();

    /* Temporarily stop VM to avoid scheduling during reset */
    vcore->state = SCHED_VCORE_STOPPED_STATE;

    enableSchedulerInterrupts();

    /* If currently running, switch to another VM first */
    if (G_HVCB->current_vm == vmId)
    {
        Scheduler_AdvanceSlot(G_HVCB, G_SCHED_CB);
    }

    /* Reinitialize VM execution context */
    initializeVMContext(vmId);

    /* Make VM ready for scheduling again */
    vcore->state = SCHED_VCORE_READY_STATE;

    return 1;
}

/**
 * @brief Retrieves the current state of a VM
 *
 * @param vmId Virtual machine ID
 * @return Current state of the VM, or INVALID state if input is incorrect
 */
VcoreStateType getVMStatus(VM_Id_t vmId)
{
    if ((G_HVCB == NULL_PTR) || (vmId >= G_HVCB->no_of_Vcores))
    {
        return SCHED_VCORE_INVALID_STATE;
    }

    return G_HVCB->Vcore_list[vmId].state;
}

/**
 * @brief Gets the ID of the currently running VM
 *
 * @return VM ID of the currently executing VM,
 *         or invalid value if the system is not initialized
 */
VM_Id_t getCurrentVMID(void)
{
    if (G_HVCB == NULL_PTR)
    {
        return (VM_Id_t)0xFF;
    }

    return G_HVCB->current_vm;
}
