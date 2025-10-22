# 🧠 Compiler Optimization Project

This project was developed as part of the **Compiler Construction** course at the **Faculty of Mathematics, University of Belgrade**.  
It implements several classical **intermediate code optimizations** used in modern compilers.

---

## ⚙️ Implemented Optimizations

1. **Constant Propagation**  
   Replaces variables that have known constant values with those constants throughout the code.

2. **Constant Folding**  
   Simplifies constant expressions at compile time (e.g., `3 + 4` → `7`).

3. **Dead Code Elimination (DCE)**  
   Removes statements and instructions whose results are never used.

4. **Common Subexpression Elimination (CSE)** *(based on dominator analysis)*  
   Detects and eliminates redundant computations that yield the same value, using dominator relationships in the control flow graph.

5. **Dead Basic Block Elimination** *(based on dominator analysis)*  
   Removes unreachable or redundant basic blocks identified through the dominator tree.

---

## 👩‍💻 Authors

**Miona Sretenović**  
**Ognjen Marković**  
Faculty of Mathematics, University of Belgrade  

---

💡 *This project demonstrates how compiler optimization techniques improve program efficiency and reduce redundancy in intermediate code.*
