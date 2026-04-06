= Recursion
Recursion is a super power! [Thorsten] However it introduces some interesting problems for the marker tree. Consider a function like [Figure x] 

#grid(
  columns: (1fr, 1fr),
  rows: 2,
  text(
    align(center, 
      ```c
      int f(int a) {
        if (a == 0) return 0;             

        return f(a-1);
      }
      ```
    )
  )
  ,

  text(""),
  text(
    align("Figure x: Recursive function", center)
  )
)

Here we could have a call tree of f(3) -> f(2) -> f(1) -> f(0), however when we break on ```c if (a == 0) return 0; ```how are we to know which instance of f we want to evaluate. There is a couple options we can search through the markers at this address and then find the one with the lowest cfa (todo: need ref to cfa somewhere) i.e. the one called last, alternatively, and the way Anura tracks this is to have markers and breakpoints store their cfa along with the address which allows for search these as a key together. We can further simplify this by giving certain types of breakpoints callbacks that are executed on hitting them, and by storing the linked marker with this callback we can call a function on a certain instance of f when the cfa and address are right.

= Looped control flow
Another control flow that 'kills' branches of the graph is any control flow that goes back to something that was executed before, this is very easy to see in loops. [Figure y] shows an example. If we want
to break when g is some value e.g. true, then we cannot break during the loop as we don't know that we're at the last iteration. This is also visible in while loops, as we cannot break as there may be
another instance of the loop, but the condition for that cannot be evaluated in the loop as it may be affected by it. Therefore any control flow that loops like this must be marked as dead and the stack
trace be saved.

#grid(
  columns: (1fr, 1fr),
  rows: 2,
  gutter: 10pt,

  figure[
      ```c
      bool g() {
          bool a = true;
          for (int i = 0; i < 10; i++) {
              if (i % 2 == 0) a = false;
              else a = true;
          }
          return a;
      }
      ```
  ],

  figure[
      ```c
      bool h() {
          bool a = true;
          while (h2()):
              if (h3()) a = false;
          return a
      }
      ```
  ],

  text(
    align(center, "Figure z: Returing control flow; While loop")
  ),
  text(
    align(center, "Figure y: Returing control flow; For loop")
  )
)
#columns(2, gutter: 8pt)[
    #colbreak()


    

]