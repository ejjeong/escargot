// escargot-skip: yield not supported
/*
 * Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/licenses/publicdomain/
 * Contributor: Jason Orendorff
 */
function f(a, b, c, d) {
    yield arguments.length;
}
reportCompare(0, f().next(), "bug 530879");
