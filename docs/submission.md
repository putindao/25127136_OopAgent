# Submission Guide

## Git status

- **11 commits**, one per build phase (≥6 required for a 1-person team). `git log --oneline`
  shows a clean incremental story: setup → client → tools → skills → agent loop → harness →
  benchmark → environment → C++26 → UML → docs.

> ⚠️ **Commit timing.** The brief wants commits spread over the weeks (≤7 days between
> consecutive commits, a feature each week). If these commits are clustered in time, keep
> committing as you review/extend in the coming weeks so the history reflects ongoing work.

## Submit a private repo with a read-only PAT (for grading)

1. Create a **private** GitHub repo and push:
   ```bash
   git remote add origin https://github.com/<you>/<repo>.git
   git push -u origin master
   ```
2. Create a **read-only** Personal Access Token:
   - Fine-grained token → *Only select repositories* → this repo →
     **Repository permissions: Contents = Read-only** (and Metadata = Read). Nothing else.
   - GitHub → Settings → Developer settings → Personal access tokens.
3. Give the instructor the repo URL + the token (per the course's submission channel).

## Package the ZIP (if uploading to LMS/Moodle)

Folder/zip name: `MSSV_OopAgent.zip` (1 member) — or `MSSV1_MSSV2[_MSSV3]_OopAgent.zip`.

```bash
# from the repo root, exclude build artifacts and VCS internals
git archive --format=zip --output ../MSSV_OopAgent.zip HEAD
```

`git archive` automatically excludes `build/`, `.git/`, and anything in `.gitignore`.

## Pre-submission checklist (maps to the rubric)

- [ ] **OOP design (25):** UML in [uml.md](uml.md); 4 patterns; no invariant violations.
- [ ] **C++ (20):** [cpp-features.md](cpp-features.md) — 8/3/2/1 across C++17/20/23/26;
      smart pointers, no leaks; meaningful exceptions.
- [ ] **Functionality (25):** 5 tools work; agent loop + loop detection; skills load/inject;
      harness + trajectory output. (`ctest` green; `./agent "..."` runs.)
- [ ] **Benchmark (15):** 10 tasks; evaluators run; JSON valid; success rate analysed
      ([benchmark-results.md](benchmark-results.md)).
- [ ] **Docs (15):** [README](../README.md) build/run/Ollama; [report](report.md); slides
      ([slides-outline.md](slides-outline.md)).
- [ ] Rename the root folder to the `MSSV...` convention before zipping.
- [ ] Fill in real MSSV(s) in the title slide and README.

## Build sanity before submitting

```bash
cmake -S . -B build -G Ninja && cmake --build build
cd build && ctest --output-on-failure       # all unit tests pass
ollama serve & ./run_eval                    # benchmark success rate
```
