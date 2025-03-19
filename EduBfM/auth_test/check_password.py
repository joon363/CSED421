import yaml
import requests

def authentification(auth_url):
    print('Authentification...')

    with open('../auth.yaml') as f:
        account_info = yaml.load(f, Loader=yaml.FullLoader)

        global student_id
        global password 
        student_id = account_info['student_id']
        password = account_info['password']

        json_account = {"student_id": student_id, "password": password}
        result = requests.post(auth_url, json = json_account)

        if "ERROR" not in result.text:
            print("Nickname: " + result.text)
            print('Done')
        else:
            print('Auth Failed')
            print(result.text)

with open('setting.yaml') as f:
    settings = yaml.load(f, Loader=yaml.FullLoader)
    authentification(settings['auth_url'])