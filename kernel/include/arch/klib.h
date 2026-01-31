#pragma once

extern void* __copy_msg_from_user_begin;
extern void* __copy_msg_from_user_end;
extern void* __copy_msg_to_user_begin;
extern void* __copy_msg_to_user_end;

// Copies 64 bytes from user space to kernel buffer.
// Returns 0 on success, -1 on fault (bad user pointer / protection)
int copy_message_from_user(const void* user_mbuf, void* dst_kbuf);

// Copies 64 bytes from kernel buffer to user space.
// Returns 0 on success, -1 on fault.
int copy_message_to_user(const void* src_kbuf, void* user_mbuf);
