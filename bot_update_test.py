import requests

api_token = "lip_iXJerL75e71PxLlQwsL3"
url = "https://lichess.org/api/bot/account/upgrade"

response = requests.post(url, headers={"Authorization": f"Bearer {api_token}"})

if response.status_code == 200:
    print("成功！帳號已升級為 BOT。")
else:
    print(f"失敗：{response.text}")