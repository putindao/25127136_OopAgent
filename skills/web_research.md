---
name: web_research
description: Answer a knowledge question by searching the web and remembering findings
keywords: [research, search, web, look up, find out, who is, what is, information, facts, learn about, lookup]
---

# Web Research

For a question you cannot answer from the conversation alone, gather evidence first.

1. **Turn the question into a focused query.** Use the key entity or phrase, not the
   whole sentence. "Who founded SQLite" -> query "SQLite founder".
2. **Search** with the `web_search` tool. Read the returned summary critically; it may
   be partial or empty.
3. **Refine if needed.** If the first result is empty or off-topic, rephrase the query
   once or twice (broader or narrower) instead of giving up.
4. **Remember what matters.** When you find a durable fact worth reusing later, store it
   with `memory_save` so a future task can recall it with `memory_search` instead of
   searching again.
5. **Synthesize, then cite the source of the claim.** Answer the original question in your
   own words and make clear which part came from the search versus your own reasoning.

Do not invent facts. If the search yields nothing usable, say so plainly rather than
guessing.
