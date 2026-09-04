; Demonstrates constants, embedded data, arithmetic, branching, and memory transfer.

.EQU COMPARISON_VALUE, 0x2A
.EQU FALLTHROUGH_VALUE, 0xFF
.EQU STORED_VALUE, 0x5A
.EQU CLEARED_VALUE, 0x00
.EQU DATA_ADDRESS, 0x80

start:
  LDI A, COMPARISON_VALUE
  LDI B, COMPARISON_VALUE
  SUB A, B
  JZ memory_demo
  LDI A, FALLTHROUGH_VALUE

memory_demo:
  LDA initial_data
  STA DATA_ADDRESS
  LDI A, CLEARED_VALUE
  LDA DATA_ADDRESS
  HALT

  initial_data:
  .BYTE STORED_VALUE
