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
;;; $Id: lush.el,v 1.13 2007/09/19 23:15:29 profshadoko Exp $
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; this file contains useful definitions for emacs

(require 'pcomplete)

;; if this is set to nil then tab completion will read the environment
;; from the lush process every time, even when nothing new was defined
(defvar cache-symbols t)

(defvar *lush-symbols* ())
(defvar *lush-symbols-stale?* t)

(defun lush-proc ()
  (condition-case err
      (inferior-lisp-proc)
    (error ())))

;; call this with M-X lush
(defun lush2 ()
  "starts Lush2 in another subwindow"
  (interactive)
  (switch-to-buffer-other-window "*inferior-lisp*")
  (inferior-lisp "lush2")
  (other-window -1)
  (set-buffer "*inferior-lisp*")
  (setq comint-prompt-regexp "^?")
  (setq comint-get-old-input #'lush-get-incomplete-input)
  (if cache-symbols
      (progn
        ;; override comint-send-string to check for definitions/file loads
        ;; entered by hand in the repl or through the lisp-load-file command
        (defvar prev-comint-send-string (symbol-function 'comint-send-string))
        (defun comint-send-string (proc inp)
          (let ((stale-re "load\\|\\^L\\|def\\|dmd\\|set\\|(d[efmz][\\t\\n ]"))
            (if (and (eq proc (lush-proc))
                     (string-match stale-re inp))
                (setq *lush-symbols-stale?* t)))
          (funcall prev-comint-send-string proc inp))

        ;; override comint-send-region to mark the symbol cache stale
        ;; whenever the lisp-eval-* commands are used
        (defvar prev-comint-send-region (symbol-function 'comint-send-region))
        (defun comint-send-region (proc start end)
          (setq *lush-symbols-stale?* t)
          (funcall prev-comint-send-region proc start end))))
    
  (set (make-local-variable 'pcomplete-parse-arguments-function)
       'lush-parse-arguments)

  (local-set-key "\r" 'lush-filter-input)
  (local-set-key "\n" 'lush-filter-input)
  (local-set-key "\t" 'pcomplete))

(global-set-key "\C-xg" 'goto-line)


(defun lush-avail? (mark)
  (let ((prompt (save-excursion
                  (search-backward-regexp "^?" nil t))))
    (and prompt (= mark (+ prompt 2)))))

(defun lush-filter-input ()
  (interactive)
  (let ((proc (lush-proc)))
    (if (and proc (eq (current-buffer) (process-buffer proc)))
        (progn
          (insert "\n")
          (let ((cur (point))
                (end (point-max))
                (mark (process-mark proc)) )
            (if (lush-avail? mark)
                ;; we're at the repl, not in debug or other i/o
                (let ((status (with-syntax-table lisp-mode-syntax-table
                                (parse-partial-sexp mark end))))
                  (if (and (= (car status) 0)   ; balanced parens
                           (not (elt status 3)) ; inside string
                           (not (elt status 4)) ; inside comment
                           (<= (point) cur) )   ; not inside the expression
                      (let ((expr (buffer-substring 
                                   (if (elt status 2) (elt status 2) (- (point) 1))
                                   (- (point) 1))))
                        (comint-send-string proc (concat expr "\n"))
                        (comint-add-to-input-history expr)
                        (command-execute 'comint-set-process-mark))
                    (goto-char cur)))
              ;; just send the input to lush
              (let ((expr (buffer-substring-no-properties mark (- end 1))))
                (comint-send-string proc (concat expr "\n"))
                (comint-add-to-input-history expr)
                (command-execute 'comint-set-process-mark))))))))

(defun filter (p lst)
  (let ((ret ()))
    (mapc (lambda (x) (if (funcall p x) (setq ret (cons x ret)))) lst)
    (nreverse ret)))

;; be careful; calling this when lush isn't available will cause this to hang
(defun lush-symbols ()
  (let ((proc (lush-proc)))
    (if proc
        (car (read-from-string
              (car (comint-redirect-results-list-from-process
                    proc "(symblist)" "(.*)" 0))))
      ())))

(defun lush-complete (sym)
  (let* ((sym-name (downcase (if (stringp sym) sym (symbol-name sym))))
         (minl (length sym-name))
         (proc (lush-proc)) )
    ;; if we have the prompt and the symbols are stale, update *lush-symbols*
    (if (and *lush-symbols-stale?*
             proc
             (eq (current-buffer) (process-buffer proc))
             (lush-avail? (process-mark proc)))
        (progn
          (setq *lush-symbols* (lush-symbols))
          (if cache-symbols
              (setq *lush-symbols-stale?* ()))))
    (filter (lambda (s)
              (and (>= (length s) minl)
                   (string= sym-name (substring s 0 minl))))
            *lush-symbols*)))

(defun lush-get-incomplete-input ()
  (let ((proc (lush-proc)))
    (if (and proc (eq (current-buffer) (process-buffer proc)))
        (buffer-substring (process-mark proc) (point-max))
      "")))

;; see http://www.emacswiki.org/cgi-bin/wiki/PcompleteExamples
(defun lush-parse-arguments ()
  (save-excursion
    (let* ((cur (point))
           (beg (search-backward-regexp "[][{}#():'\" \t\n]" nil t))
           (pos (if beg (+ beg 1) cur))
           (arg (buffer-substring-no-properties pos cur)))
      (cons (list "lush-complete" arg)
            (list (point-min) pos)))))

(defun pcomplete/lush-complete ()
  "Complete the symbol at point"
   (let* ((sym (cadr (car (lush-parse-arguments))))
          (completions (lush-complete sym)))
     (if completions
         (throw 'pcomplete-completions completions)
       (pcomplete-here (pcomplete-all-entries)))))

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
(put 'for 'lisp-indent-function 1)
(put 'for* 'lisp-indent-function 1)
(put 'do 'lisp-indent-function 1)
(put 'domapc 'lisp-indent-function 1)
(put 'domapcar 'lisp-indent-function 1)
(put 'domapcan 'lisp-indent-function 1)
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
(put 'defcmacro 'list-indent-function 3)
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
		"domapc" "domapcar" "domapcan"
                "while" "let" "lete" "let*" "progn" "prog1" 
                "let-filter" "reading" "writing" "selectq"
                "do-while" "mapwhile" "mapfor" "repeat"
                "idx-eloop" "idx-bloop" "idx-gloop"
                "ifdef" "ifcompiled"
                "lambda" "flambda" "mlambda" ) t) )
        1)
  (list
   (concat "(\\(de\\|df\\|dmc\\|dmd\\|dz\\|dm\\|dhm-p\\|dhm-t\\|dhm-c\\|defcmacro\\)"
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
   (concat "(\\(\\(def\\)template\\)"
           "[ \t]+\\(\\sw+\\||[^|]+|\\)?"
           "[ \t]+\\(\\sw+\\||[^|]+|\\)?")
   '(1 font-lock-keyword-face)
   '(3 font-lock-type-face nil t)
   '(4 font-lock-type-face nil t) )
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


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; some Lush-related macros
;; Author: Jan Kreps (krepsj(at)seznam.cz)
;; Keywords: lush

;;; Hopefully, this will evolve into a minor mode. 
;;; you can type M-x lush-new-method or set your own key
;;; binding for it.

;;; Code: Functions for inserting method declarations with comments at
;;; once.

;;; TODO: Function for commenting or uncommenting regions of code with
;;; one keystroke according to the context. 
;;;
;;; Maybe rename all "internal" functions and add "lush-" prefix to
;;; them.
;;;
;;; Add some nifty menu.
;;;
;;; Automagically switch between lisp mode and C mode
;;; according to the currently edited content

;; -----------------------------------------------------------------------
;; Functions and variables for filling method prototype according to
;; the last declared class name.  Basically it searches backward for
;; string contained in variable class-identifier. After finding it we
;; save first next word as a lush-class-name. Then it is used for
;; constructing comment string and prototype. Comments can be turned
;; off by setting lush-comments-on variable to nil.


(defvar lush-class-identifier "defclass"
  "String that identifies begin of class declaration. Default value is
'defclass. Overwriting this variable is neither expected nor
necessary, but is stil possible. ")

(defvar lush-method-identifier "defmethod"
  "Identifies begining of method declaration. Default value is
'defmethod. As with 'lush-class-identifier, changes of it's value are
not expected.")

(defvar lush-class-name "lush-class-name"
  "Variable for saving found class name. Default value is
'lush-class-name. Used by several functions.")

(defvar lush-method-name "lush-method-name"
  "Variable for saving entered method name. Default value is
'lush-method-name -- should be overwriten.")

(defvar lush-method-params ()
  "Variable holds list of parameters for current method. Default is
empty list.")

(defvar lush-comments-on t
  "Variable determining whether new method/functions will be commented or
not. Default value is 't.")


(defun next-word ()
  "Returns next word after current position. This position could be
find using search functions. Purpose is to get word following the
searched one. Words with hyphens and underscores are accepted (eg. not
splitted) as they can be valid function names. "
  (let (substring 		
	start			
	end
	)
    (save-excursion
      (setq start (point))
      (forward-word-dash)   	
      (setq end (point))	
      (setq substring (buffer-substring-no-properties start end))) 
    substring) 				
  )
    

(defun forward-word-dash ()
   "Simmilar like ordinary 'forward-word but does not consider dashes
and underscores as words separator."
   (let (foo
	 )
     (forward-word 1)
     (setq foo (char-to-string (following-char)))
     (if (or (string= foo "-") (string= foo "_")) 
	 (forward-word-dash) 
       )                     
     )
   )


(defun search-class-name ()
  "Searches backward for class name. Found name is saved in
lush-class-name variable and returned as function value as
well. Complains if there is no preceeding defclass."
  (save-excursion
    (if ( not (search-backward lush-class-identifier () t)) 
	(error "No previously declared class found")
      (progn
	(forward-word 1)		          
	(skip-chars-forward " ")		  
	(setq lush-class-name (next-word))	  
	)
      )
    ;; return value purely for aesthetic reasons :-)
    lush-class-name
    )
  )



(defun insert-lush-method ()
  "Begins new method declaration. First it searches backward for last
defclass statement and finds name of this class. Then uses this name
in declaration of actual method. Commenting of this prototype depends
on variable lush-commnents-on. If true then first is inserted
documentation string #? (==> lush-class-name lush-method-name). 
Variable lush-method-name should be set prior this function is
called."
  (let ((args (split-string lush-method-params))
	(comment "")
	)
     
    (while (> (length args) 0)
      (setq comment (concat comment " <" (pop args) ">"))
      ) 
    (if lush-comments-on
	(progn
	  (if (string= lush-class-name lush-method-name)  ; constructor? 	
	      (insert "#? (new " lush-class-name  comment ")")   
	    (insert "#? (==> " lush-class-name " "  lush-method-name  comment ")") 
	    )
	  (newline)
	  (insert ";;")
	  (newline)
	  ) 
      )
    (insert "("lush-method-identifier " " lush-class-name " " lush-method-name " (" lush-method-params ")")
    (newline)
    )
  )


(defun lush-new-method ()
  "Asks for method name and for its params. Then searches for last
declared class, finds its name and puts new text in buffer in usual
form:

#?(==> classname methodname <foo> <bar>)
;;
\(defmethod classname methodname (foo bar)

In case of constructor comment line will be:

#? (new classname <foo> <bar>)

Commenting can be turned off by setting variable 'lush-comments-on to
nil.
"
  (interactive)

  (setq lush-class-name (search-class-name))
  (setq lush-method-name (read-string "Function name: ")) 
  (setq lush-method-params (read-string "Parameters: "))  
  (insert-lush-method)		      		      
  )

