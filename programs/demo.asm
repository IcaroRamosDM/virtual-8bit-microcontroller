; Demonstrates arithmetic, branching, and memory transfer.

start:
  LDI A, 0x2A
  LDI B, 0x2A
  SUB A, B
  JZ memory_demo
  LDI A, 0xFF

memory_demo:
  LDI A, 0x5A
  STA 0x80
  LDI A, 0x00
  LDA 0x80
  HALT
