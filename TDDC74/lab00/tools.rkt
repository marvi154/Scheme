;; uppgift 5
(define myname "<Margareta>")
;;(printf "Hej ~a" myname)

;; uppgift 6
#|Felet är att scheme är ett språk som är prefix-baserat allså, 
operatorn ska vara länst fram i proceduren följt av operanderna|#
;; (- 9 (+ 2 (* -3 4)))

;; uppgift 7
;; (/ 2 3) returnerar ett rationellet tal 2/3
;; (/ 2 3.0) returnerar ett flyttal 0.66666.,...7 detta beror på att den andra operanden är ett flyttal
;; (* 3 (/ 1 3)) retunerar 1
;; (* 3 (/ 1 3.0)) retunerar 1.0, detta beror på att den andra operanden är ett flyttal


#lang racket
(provide square)
;; uppgift 8
(define square
  (lambda (n)
  ;; return n^2
    (* n n)))

;; Denna function ger ut det dubbla av inargumentet
(define double
  (lambda (n)
    (* 2*n)))