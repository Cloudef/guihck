#ifndef GUIHCKGUILEDEFAULTSCM_H
#define GUIHCKGUILEDEFAULTSCM_H

static const char GUIHCK_GUILE_DEFAULT_SCM[] =
    "(define (flatmap f xs) (apply append (map f xs)))"

    "(define this get-element)"

    "(define parent"
    "  (case-lambda"
    "    (() (parent (get-element)))"
    "    ((element)"
    "      (push-element! element)"
    "      (push-parent-element!)"
    "      (let ((parent-element (get-element)))"
    "        (pop-element!)"
    "        (pop-element!)"
    "        parent-element))))"


    "(define child"
    "  (case-lambda"
    "    ((index) (child (get-element) index))"
    "    ((element index)"
    "      (push-element! element)"
    "      (push-child-element! index)"
    "      (let ((child-element (get-element)))"
    "        (pop-element!)"
    "        (pop-element!)"
    "        child-element))))"

    "(define children"
    "  (case-lambda"
    "    (() (children (get-element)))"
    "    ((element)"
    "      (push-element! element)"
    "      (let ((n (get-element-child-count)))"
    "        (define (iter i lst)"
    "          (if (< i n)"
    "            (begin"
    "              (push-child-element! i)"
    "              (let ((c (get-element)))"
    "                (pop-element!)"
    "                (iter (+ i 1) (cons c lst))))"
    "            lst))"
    "        (let ((result (reverse! (iter 0 '()))))"
    "          (pop-element!)"
    "          result)))))"

    "(define (find-element id)"
    "  (begin"
    "    (push-element-by-id! id)"
    "    (let ((result-element (get-element)))"
    "      (pop-element!)"
    "      result-element)))"

    "(define (resolve e)"
    "  (cond ((eq? e 'parent) (parent))"
    "        ((eq? e 'this) (this))"
    "        (else (find-element e))))"

    "(define (wrap-method-to-context proc element)"
    "  (lambda args"
    "    (push-element! element)"
    "    (let ((result (apply proc args)))"
    "      (pop-element!)"
    "      result)))"

    "(define (create-elements! . elements)"
    "  (for-each (lambda (e) (e)) elements))"

    "(define (create-element type nested-args)"
    "  (define (flatten-args as)"
    "    (define (flatten-arg a)"
    "      (define (flattenable? a) (and (list? a) (eq? (car a) 'arg-list)))"
    "      (if (flattenable? a)"
    "        (flatten-args (cdr a))"
    "        (list a)))"
    "    (flatmap flatten-arg as))"

    "  (define args (flatten-args nested-args))"

    "  (define (set-id)"
    "    (define (id? d) (and (list? d) (eq? (car d) 'id)))"
    "    (for-each (lambda (id) (set-element-property! 'id (cadr id)))"
    "              (filter id? args)))"

    "  (define (eval-children)"
    "    (define child? procedure?)"
    "    (for-each (lambda (child) (child))"
    "              (filter child? args)))"

    "  (define (set-props)"
    "    (define (prop? d) (and (list? d) (eq? (list-ref d 0) 'prop)))"
    "    (define (bind? v) (and (list? v) (eq? (list-ref v 0) 'bind)))"
    "    (define (alias? v) (and (list? v) (eq? (list-ref v 0) 'alias)))"
    "    (define (method? v) (and (list? v) (eq? (list-ref v 0) 'method)))"
    "    (define (make-bind value)"
    "      (list 'bind ((list-ref value 1)) (list-ref value 2)))"
    "    (define (make-alias value)"
    "      (list 'alias (resolve (list-ref value 1)) (list-ref value 2)))"
    "    (define (make-method value)"
    "      (wrap-method-to-context (list-ref value 1) (this)))"
    "    (define (process p)"
    "      (let ((key (list-ref p 1)) (value (list-ref p 2)))"
    "         (cond ((bind? value)"
    "                (set-element-property! key (make-bind value)))"
    "               ((alias? value)"
    "                (set-element-property! key (make-alias value)))"
    "               ((method? value)"
    "                (set-element-property! key (make-method value)))"
    "               (else"
    "                (set-element-property! key value)))))"
    "    (for-each process (filter prop? args)))"

    "  (lambda ()"
    "    (push-new-element! type)"
    "    (set-id)"
    "    (eval-children)"
    "    (set-props)"
    "    (if (procedure? (get-element-property 'init))"
    "      ((get-element-property 'init)))"
    "    (pop-element!)))"

    "(define (arg-list args) (cons 'arg-list args))"

    "(define (prop key value) (list 'prop key value))"
    "(define (alias key element aliased) (prop key (list 'alias element aliased)))"
    "(define (method key proc) (prop key (list 'method proc)))"

    "(define (id value) (list 'id value))"

    "(define (composite constructor . default-args)"
    "  (lambda (. args)"
    "    (apply constructor (append default-args args))))"

    "(define set-prop!"
    "  (case-lambda"
    "    ((property value) (set-prop! (get-element) property value))"
    "    ((element property value)"
    "      (push-element! element)"
    "      (set-element-property! property value)"
    "      (pop-element!))))"

    "(define set-method!"
    "  (case-lambda"
    "    ((property value)"
    "     (set-method! (this) property value))"
    "    ((element property value)"
    "     (set-prop! element property (wrap-method-to-context value element)))))"

    "(define get-prop"
    "  (case-lambda"
    "    ((property) (get-prop (get-element) property))"
    "    ((element property)"
    "      (push-element! element)"
    "      (let ((value (get-element-property property)))"
    "        (pop-element!)"
    "        value))))"

    "(define (observe . vals)"
    "  (define (pairs lst)"
    "    (if (null? lst)"
    "      '()"
    "      (cons (cons (car lst) (cadr lst)) (pairs (cddr lst)))))"
    "  (define (process pair)"
    "    (cons (resolve (car pair)) (cdr pair)))"
    "  (map process (pairs vals)))"

    "(define bind"
    "  (case-lambda"
    "    ((property callback) (bind (this) property callback))"
    "    ((element property callback)"
    "      (add-element-property-listener! element property callback))))"

    "(define bound"
    "  (case-lambda"
    "    ((bindings) (bound bindings identity))"
    "    ((bindings callback)"
    "     (list 'bind (lambda () (apply observe bindings)) callback))))"

    "(define unbind remove-element-property-listener!)"

    "(define focus!"
    "  (case-lambda"
    "    (() (keyboard-focus!))"
    "    ((element)"
    "      (push-element! element)"
    "      (keyboard-focus!)"
    "      (pop-element!))))"

    "(define (call first . rest)"
    "  (define (do-call element property args)"
    "    (apply (get-prop element property) args))"
    "  (if (integer? first)"
    "    (do-call first (car rest) (cdr rest))"
    "    (do-call (get-element) first rest)))"

    ;
#endif // GUIHCKGUILEDEFAULTSCM_H
