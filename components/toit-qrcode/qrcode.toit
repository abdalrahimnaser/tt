// Copyright (C) 2024 Toitware ApS.
// Use of this source code is governed by a Zero-Clause BSD license that can
// be found in the LICENSE file.

import system.external

FUNCTION-ID ::= 0

main:
  qrcode := external.Client.open "toitlang.org/tutorial-qrcode"
  response := qrcode.request FUNCTION-ID "https://toitlang.org"
  size := response[0]
  bit-pos := 8
  size.repeat: | x |
    line := ""
    size.repeat: | y |
      byte := response[bit-pos >> 3]
      bit := byte & (1 << (bit-pos & 7))
      line += bit == 0 ? "  " : "██"
      bit-pos++
    print line
  qrcode.close
