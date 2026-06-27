# Implementation Plan - Phase 2 & 3: Semantic Processing & Visuals

This plan covers the integration of the Gemini LLM and the formatting of Discord responses.

## 1. Process Layer (Gemini Integration)
The goal is to convert informal strings (e.g., "Spent 50 bucks on sushi") into a structured `Expense` object.

### 1.1 Gemini Client Implementation
- Use `dpp::https_client` or a lightweight wrapper to hit `https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent`.
- Securely fetch `GEMINI_API_KEY` from environment variables.

### 1.2 Prompt Engineering
- System Instruction: "You are a financial parser. Extract amount, currency, category, and description from the user text. Return strictly JSON."
- Example: `{ "amount": 50.0, "currency": "USD", "category": "Food", "description": "sushi" }`

### 1.3 `Processor::process_message`
- Input: `raw_content`, `user_id`.
- Output: `bool` (Success/Fail).
- Logic:
    1. HTTP Post to Gemini.
    2. JSON Parsing (using D++'s `nlohmann/json`).
    3. `db_.save_expense(parsed_expense)`.

## 2. Reply Layer (Discord Embeds)
- Create `Replier` class.
- Map categories to emojis (e.g., Food -> 🍕).
- Use `dpp::embed` to build the transaction receipt.

## 3. Receive Layer (Slash Commands)
- Register `/expense <query>` command.
- Offload processing to a background thread to prevent blocking the D++ Heartbeat.

## 4. Current Progress
- [x] Database Schema & Migrations.
- [x] Docker Infrastructure.
- [x] Basic Main & Cluster Setup.
- [x] Gemini API Integration.
- [x] JSON Parsing Logic.
- [x] WorkerPool & Queue Logic.
- [x] Production Docker Infrastructure.
- [x] GitLab CI/CD Pipeline.
- [x] Local Deployment Management (Makefile).
- [x] Replier Visual Cards (Rich Embeds).
