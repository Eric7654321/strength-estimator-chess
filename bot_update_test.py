import requests

api_token = "lip_iXJerL75e71PxLlQwsL3"
url = "https://lichess.org/api/bot/account/upgrade"

response = requests.post(url, headers={"Authorization": f"Bearer {api_token}"})

if response.status_code == 200:
    print("Success: Bot account upgraded to premium.")
else:
    print(f"Fatal：{response.text}")
