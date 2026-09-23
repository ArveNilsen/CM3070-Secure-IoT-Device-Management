### Full gateway reset (start from scratch)

Wipes all enrollment and trust-store state. Use for a clean slate during
development/testing.

- Stop the gateway process first.
- Run the reset script:
```bash
python3 gateway/reset_gateway.py
```
- See script help for options.
- Restart the gateway
- Re-register devices
