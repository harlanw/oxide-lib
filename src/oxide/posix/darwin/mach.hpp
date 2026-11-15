#pragma once

#include <mach/kern_return.h>

#include <oxide/core.hpp>
#include <oxide/compiler/macros.hpp>

namespace oxide::posix::darwin {

class KernelError {
public:
    enum struct Error : i32 {
        SUCCESS = KERN_SUCCESS,
        /// Specified address is not currently valid.
        INVALID_ADDRESS = KERN_INVALID_ADDRESS,
        /// Specified memory is valid, but does not permit the required forms of access.
        PROTECTION_FAILURE = KERN_PROTECTION_FAILURE,
        /// The address range specified is already in use, or no address range of the size specified
        /// could be found.
        NO_SPACE = KERN_NO_SPACE,
        /// The function requested was not applicable to this type of argument, or an argument is
        /// invalid
        INVALID_ARGUMENT = KERN_INVALID_ARGUMENT,
        /// The function could not be performed. A catch-all.
        FAILURE = KERN_FAILURE,
        /// A system resource could not be allocated to fulfill this request. This failure may not
        /// be permanent.
        RESOURCE_SHORTAGE = KERN_RESOURCE_SHORTAGE,
        /// The task in question does not hold receive rights for the port argument.
        NOT_RECEIVER = KERN_NOT_RECEIVER,
        /// Bogus access restriction.
        NO_ACCESS = KERN_NO_ACCESS,
        /// During a page fault, the target address refers to a memory object that has been
        /// destroyed. This failure is permanent.
        MEMORY_FAILURE = KERN_MEMORY_FAILURE,
        /// During a page fault, the memory object indicated that the data could not be returned.
        /// This failure may be temporary; future attempts to access this same data may succeed, as
        /// defined by the memory object.
        MEMORY_ERROR = KERN_MEMORY_ERROR,
        /// The receive right is already a member of the portset.
        ALREADY_IN_SET = KERN_ALREADY_IN_SET,
        /// The receive right is not a member of a port set.
        NOT_IN_SET = KERN_NOT_IN_SET,
        /// The name already denotes a right in the task.
        NAME_EXISTS = KERN_NAME_EXISTS,
        /// The operation was aborted. Ipc code will catch this and reflect it as a message error.
        ABORTED = KERN_ABORTED,
        /// The name doesn't denote a right in the task.
        INVALID_NAME = KERN_INVALID_NAME,
        /// Target task isn't an active task.
        INVALID_TASK = KERN_INVALID_TASK,
        /// The name denotes a right, but not an appropriate right.
        INVALID_RIGHT = KERN_INVALID_RIGHT,
        /// A blatant range error.
        INVALID_VALUE = KERN_INVALID_VALUE,
        /// Operation would overflow limit on user-references.
        UREFS_OVERFLOW = KERN_UREFS_OVERFLOW,
        /// The supplied (port) capability is improper.
        INVALID_CAPABILITY = KERN_INVALID_CAPABILITY,
        /// The task already has send or receive rights for the port under another name.
        RIGHT_EXISTS = KERN_RIGHT_EXISTS,
        /// Target host isn't actually a host.
        INVALID_HOST = KERN_INVALID_HOST,
        /// An attempt was made to supply "precious" data for memory that is already present in a
        /// memory object.
        MEMORY_PRESENT = KERN_MEMORY_PRESENT,
        /// A page was requested of a memory manager via memory_object_data_request for an object
        /// using a MEMORY_OBJECT_COPY_CALL strategy, with the VM_PROT_WANTS_COPY flag being used to
        /// specify that the page desired is for a copy of the object, and the memory manager has
        /// detected the page was pushed into a copy of the object while the kernel was walking the
        /// shadow chain.
        MEMORY_DATA_MOVED = KERN_MEMORY_DATA_MOVED,
        /// A strategic copy was attempted of an object upon which a quicker copy is now possible.
        /// The caller should retry the copy using vm_object_copy_quickly. This error code is seen
        /// only by the kernel.
        MEMORY_RESTART_COPY = KERN_MEMORY_RESTART_COPY,
        /// An argument applied to assert processor set privilege was not a processor set control
        /// port.
        INVALID_PROCESSOR_SET = KERN_INVALID_PROCESSOR_SET,
        /// The specified scheduling attributes exceed the thread's limits.
        POLICY_LIMIT = KERN_POLICY_LIMIT,
        /// The specified scheduling policy is not currently enabled for the processor set.
        INVALID_POLICY = KERN_INVALID_POLICY,
        /// The external memory manager failed to initialize the memory object.
        INVALID_OBJECT = KERN_INVALID_OBJECT,
        /// A thread is attempting to wait for an event for which there is already a waiting thread.
        ALREADY_WAITING = KERN_ALREADY_WAITING,
        /// An attempt was made to destroy the default processor set.
        DEFAULT_SET = KERN_DEFAULT_SET,
        /// An attempt was made to fetch an exception port that is protected, or to abort a thread
        /// while processing a protected exception.
        EXCEPTION_PROTECTED = KERN_EXCEPTION_PROTECTED,
        /// A ledger was required but not supplied.
        INVALID_LEDGER = KERN_INVALID_LEDGER,
        /// The port was not a memory cache control port.
        INVALID_MEMORY_CONTROL = KERN_INVALID_MEMORY_CONTROL,
        /// An argument supplied to assert security privilege was not a host security port.
        INVALID_SECURITY = KERN_INVALID_SECURITY,
        /// A thread is not currently depressed.
        NOT_DEPRESSED = KERN_NOT_DEPRESSED,
        /// Object has been terminated and is no longer available
        TERMINATED = KERN_TERMINATED,
        /// Lock set has been destroyed and is no longer available
        LOCK_SET_DESTROYED = KERN_LOCK_SET_DESTROYED,
        /// The thread holding the lock terminated before releasing the lock
        LOCK_UNSTABLE = KERN_LOCK_UNSTABLE,
        /// The lock is already owned by another thread
        LOCK_OWNED = KERN_LOCK_OWNED,
        /// The lock is already owned by the calling thread
        LOCK_OWNED_SELF = KERN_LOCK_OWNED_SELF,
        /// Semaphore has been destroyed and is no longer available
        SEMAPHORE_DESTROYED = KERN_SEMAPHORE_DESTROYED,
        /// Return from RPC indicating the target server was terminated before it successfully
        /// replied
        RPC_SERVER_TERMINATED = KERN_RPC_SERVER_TERMINATED,
        /// Terminate an orphaned activation.
        RPC_TERMINATE_ORPHAN = KERN_RPC_TERMINATE_ORPHAN,
        /// Allow an orphaned activation to continue executing.
        RPC_CONTINUE_ORPHAN = KERN_RPC_CONTINUE_ORPHAN,
        /// The function requested was not applicable to this type of argument, or an argument is
        /// invalid
        NOT_SUPPORTED = KERN_NOT_SUPPORTED,
        /// Remote node down or inaccessible.
        NODE_DOWN = KERN_NODE_DOWN,
        /// A signalled thread was not actually waiting.
        NOT_WAITING = KERN_NOT_WAITING,
        /// Some thread-oriented operation (semaphore_wait) timed out
        OPERATION_TIMED_OUT = KERN_OPERATION_TIMED_OUT,
        /// During a page fault, indicates that the page was rejected as a result of a signature
        /// check.
        CODESIGN_ERROR = KERN_CODESIGN_ERROR,
        /// The requested property cannot be changed at this time.
        POLICY_STATIC = KERN_POLICY_STATIC,
        /// The provided buffer is of insufficient size for the requested data.
        INSUFFICIENT_BUFFER_SIZE = KERN_INSUFFICIENT_BUFFER_SIZE,
        /// Denied by security policy
        DENIED = KERN_DENIED,
        /// The KC on which the function is operating is missing
        MISSING_KC = KERN_MISSING_KC,
        /// The KC on which the function is operating is invalid
        INVALID_KC = KERN_INVALID_KC,
        /// A search or query operation did not return a result
        NOT_FOUND = KERN_NOT_FOUND,
        /// Access to an guarded object slot attempted
        INVALID_GUARD_OBJECT_SLOT = KERN_INVALID_GUARD_OBJECT_SLOT,
        /// Maximum return value allowable
        RETURN_MAX = KERN_RETURN_MAX,
        UNKNOWN = KERN_RETURN_MAX + 1,
    };

    REEXPORT_ERROR(SUCCESS);
    REEXPORT_ERROR(INVALID_ADDRESS);
    REEXPORT_ERROR(PROTECTION_FAILURE);
    REEXPORT_ERROR(NO_SPACE);
    REEXPORT_ERROR(INVALID_ARGUMENT);
    REEXPORT_ERROR(FAILURE);
    REEXPORT_ERROR(RESOURCE_SHORTAGE);
    REEXPORT_ERROR(NOT_RECEIVER);
    REEXPORT_ERROR(NO_ACCESS);
    REEXPORT_ERROR(MEMORY_FAILURE);
    REEXPORT_ERROR(MEMORY_ERROR);
    REEXPORT_ERROR(NAME_EXISTS);
    REEXPORT_ERROR(ABORTED);
    REEXPORT_ERROR(INVALID_NAME);
    REEXPORT_ERROR(INVALID_TASK);
    REEXPORT_ERROR(INVALID_RIGHT);
    REEXPORT_ERROR(INVALID_VALUE);
    REEXPORT_ERROR(UREFS_OVERFLOW);
    REEXPORT_ERROR(INVALID_CAPABILITY);
    REEXPORT_ERROR(RIGHT_EXISTS);
    REEXPORT_ERROR(INVALID_HOST);
    REEXPORT_ERROR(MEMORY_PRESENT);
    REEXPORT_ERROR(MEMORY_DATA_MOVED);
    REEXPORT_ERROR(MEMORY_RESTART_COPY);
    REEXPORT_ERROR(INVALID_PROCESSOR_SET);
    REEXPORT_ERROR(POLICY_LIMIT);
    REEXPORT_ERROR(INVALID_POLICY);
    REEXPORT_ERROR(INVALID_OBJECT);
    REEXPORT_ERROR(ALREADY_WAITING);
    REEXPORT_ERROR(DEFAULT_SET);
    REEXPORT_ERROR(EXCEPTION_PROTECTED);
    REEXPORT_ERROR(INVALID_LEDGER);
    REEXPORT_ERROR(INVALID_MEMORY_CONTROL);
    REEXPORT_ERROR(INVALID_SECURITY);
    REEXPORT_ERROR(NOT_DEPRESSED);
    REEXPORT_ERROR(TERMINATED);
    REEXPORT_ERROR(LOCK_SET_DESTROYED);
    REEXPORT_ERROR(LOCK_UNSTABLE);
    REEXPORT_ERROR(LOCK_OWNED);
    REEXPORT_ERROR(LOCK_OWNED_SELF);
    REEXPORT_ERROR(SEMAPHORE_DESTROYED);
    REEXPORT_ERROR(RPC_SERVER_TERMINATED);
    REEXPORT_ERROR(RPC_TERMINATE_ORPHAN);
    REEXPORT_ERROR(RPC_CONTINUE_ORPHAN);
    REEXPORT_ERROR(NOT_SUPPORTED);
    REEXPORT_ERROR(NODE_DOWN);
    REEXPORT_ERROR(NOT_WAITING);
    REEXPORT_ERROR(OPERATION_TIMED_OUT);
    REEXPORT_ERROR(CODESIGN_ERROR);
    REEXPORT_ERROR(POLICY_STATIC);
    REEXPORT_ERROR(INSUFFICIENT_BUFFER_SIZE);
    REEXPORT_ERROR(DENIED);
    REEXPORT_ERROR(MISSING_KC);
    REEXPORT_ERROR(INVALID_KC);
    REEXPORT_ERROR(NOT_FOUND);
    REEXPORT_ERROR(INVALID_GUARD_OBJECT_SLOT);
    REEXPORT_ERROR(RETURN_MAX);
    REEXPORT_ERROR(UNKNOWN);

    constexpr KernelError(kern_return_t error) : _error(static_cast<Error>(error)) {}

    constexpr Error match() const { return _error; }

    bool is_success() const { return _error == SUCCESS; }
    bool is_timed_out() const { return _error == OPERATION_TIMED_OUT; }

    constexpr bool operator==(const KernelError& other) const { return _error == other._error; }
    constexpr bool operator!=(const KernelError& other) const { return _error != other._error; }
    constexpr bool operator==(const Error error) const { return _error == error; }
    constexpr bool operator!=(const Error error) const { return _error != error; }

private:
    Error _error;

    constexpr KernelError(Error error) : _error(error) {}
};

class SyncPolicy final {
public:
private:
};

} // namespace oxide::posix::darwin
