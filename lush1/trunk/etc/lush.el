;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; LUSH Lisp Universal Shell
;;;   Copyright (C) 2002 Leon Bottou, Yann Le Cun, AT&T Corp, NECI.
;;; Includes parts of TL3:
;;;   Copyright (C) 1987-1999 Leon Bottou and Neuristique.
;;; Includes selected parts of SN3.2:
;;;   Copyright (C) 1991-2001 AT&T Corp.
;;;
;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program; if not, write to the Free Software
;;; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; $Id: lush.el,v 1.3 2002-07-15 19:47:40 leonb Exp $
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;; this file contains useful definitions for emacs

;; detection
(setq auto-mode-alist (cons (cons "\\.sn$" 'lisp-mode) auto-mode-alist))
(setq auto-mode-alist (cons (cons "\\.tl$" 'lisp-mode) auto-mode-alist))
(setq auto-mode-alist (cons (cons "\\.lsh$" 'lisp-mode) auto-mode-alist))

;; indentation
(put 'when 'lisp-indent-function 1)
(put 'ifdef 'lisp-indent-function 2)
(put 'ifcompiled 'lisp-indent-function 1)
(put 'reading 'lisp-indent-function 1)
(put 'writing 'lisp-indent-function 1)
(put 'idx-bloop 'lisp-indent-function 1)
(put 'idx-eloop 'lisp-indent-function 1)
(put 'idx-gloop 'lisp-indent-function 1)
(put 'on-error 'lisp-indent-function 1)
(put 'on-break 'lisp-indent-function 1)
(put 'all 'lisp-indent-function 1)
(put 'each 'lisp-indent-function 1)
(put 'let-filter 'lisp-indent-function 1)
(put 'mapfor 'lisp-indent-function 1)
(put 'mapwhile 'lisp-indent-function 1)
(put 'do-while 'lisp-indent-function 1)
(put 'repeat 'lisp-indent-function 1)
(put 'selectq 'lisp-indent-function 1)
(put 'de 'lisp-indent-function 2)
(put 'df 'lisp-indent-function 2)
(put 'dm 'lisp-indent-function 2)
(put 'dz 'lisp-indent-function 2)
(put 'dmd 'lisp-indent-function 2)
(put 'lambda 'lisp-indent-function 1)
(put 'flambda 'lisp-indent-function 1)
(put 'mlambda 'lisp-indent-function 1)
(put 'defmethod 'lisp-indent-function 3)
(put 'demethod 'lisp-indent-function 3)
(put 'dfmethod 'lisp-indent-function 3)
(put 'dmmethod 'lisp-indent-function 3)
(put 'dhm-p 'lisp-indent-function 2)
(put 'dhm-t 'lisp-indent-function 2)
(put 'dhm-c 'lisp-indent-function 2)

;; masquerade cmacro as comments
(modify-syntax-entry ?# "' 13b" lisp-mode-syntax-table)
(modify-syntax-entry ?{ "(}2b" lisp-mode-syntax-table)
(modify-syntax-entry ?} "){4" lisp-mode-syntax-table)

;; colors
(font-lock-add-keywords 
 'lisp-mode
 (list
  (cons (concat
         "(" (regexp-opt
              '("cond" "if" "when" "for" "each" "all"
                "while" "let" "let*" "progn" "prog1" 
                "let-filter" "reading" "writing" "selectq"
                "do-while" "mapwhile" "mapfor" "repeat"
                "idx-eloop" "idx-bloop" "idx-gloop"
                "ifdef" "ifcompiled"
                "lambda" "flambda" "mlambda" ) t) )
        1)
  (list
   (concat "(\\(de\\|df\\|dmc\\|dmd\\|dz\\|dm\\|dhm-p\\|dhm-t\\|dhm-c\\)"
           "[ \t]+\\(\\sw+\\||[^|]+|\\)?")
   '(1 font-lock-keyword-face)
   '(2 font-lock-function-name-face t t) )
  (list
   (concat "(\\(\\(def\\|de\\|df\\|dm\\)method\\)"
           "[ \t]+\\(\\sw+\\||[^|]+|\\)?"
           "[ \t]+\\(\\sw+\\||[^|]+|\\)?")
   '(1 font-lock-keyword-face)
   '(3 font-lock-type-face t t)
   '(4 font-lock-function-name-face nil t) )
  (list
   (concat "(\\(\\(def\\)class\\)"
           "[ \t]+\\(\\sw+\\||[^|]+|\\)?"
           "[ \t]+\\(\\sw+\\||[^|]+|\\)?")
   '(1 font-lock-keyword-face)
   '(3 font-lock-type-face nil t)
   '(4 font-lock-type-face nil t) )
  (list
   "\\<:\\sw\\sw+\\>" 0 (internal-find-face 'default) t t)
) )

