from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.by import By
from selenium.common.exceptions import ElementClickInterceptedException, TimeoutException


from element import *


class BasePage(object):
    def __init__(self, client):
        self.client = client


class MainPage(BasePage):
    olcBtn = OnLoadWindowClose()
    ipt = SchInputBox()
    ipt_btn = SchInputButton()
    hpoDragTgt = DragHpoTarget()
    diagBtn = GetDiseasesButton()
    ftPrepped = FeaturesPrepared()
    disRes = DiseasesResult()
    ftTab = FeatureTab()
    diagTab = DiagTab()
    diagTabClose = DiagTabClose()
    clrBtn = ClearHpoButton()
    yBtn = YesButton()
    schRes = SchResult()
    LIMIT = 10

    def __init__(self, *args, **kwargs):
        BasePage.__init__(self, *args, **kwargs)
        self.client.maximize_window()
        self.olcBtn.click()

    def hpoEle(self, hpo):
        while True:
            schRes = self.schRes
            if len(schRes) < 10:
                break
        for ele in schRes:
            if hpo in ele.text:
                return ele

    @property
    def AC(self):
        return ActionChains(self.client)

    @property
    def result(self):
        self.diagTab.click()
        WebDriverWait(self.client, 60).until(
            lambda c: self.disRes[0].text.strip() != ''
        )
        return self.__parse_diseases_result(self.disRes)

    def get_diseases(self, hpos):
        self.ftTab.click()
        for hpo in hpos:
            self.add_one_hpo(hpo)
        self.diagBtn.click()
        self.clear_ft()
        diagTabClose_l = self.diagTabClose[4:]
        for a in diagTabClose_l[:-1]:
            a.click()

    def add_one_hpo(self, hpo):
        # search one hpo
        ipt, iptBtn = self.ipt, self.ipt_btn
        ipt.clear()
        ipt.send_keys(*hpo)
        while True:
            iptBtn.click()
            # wait until it appears in the search result
            try:
                WebDriverWait(self.client, 10).until(lambda c: self.hpoEle(hpo) is not None and hpo in self.hpoEle(hpo).text)
            except:
                continue
            break
        while True:
            # add it to features on the right
            self.AC.click_and_hold(self.hpoEle(hpo)).perform()
            self.AC.move_to_element_with_offset(self.hpoDragTgt, 2, 2).perform()
            self.AC.release().perform()
            try:
                # wait until it appears on the right
                WebDriverWait(self.client, 10).until(
                    lambda c: hpo in self.ftPrepped.text
                )
            except:
                continue
            break
        try:
            yBtn = self.yBtn
        except TimeoutException as e:
            return
        yBtn.click()
            

    def clear_ft(self):
        self.ftTab.click()
        try:
            self.clrBtn.click()
            self.yBtn.click()
        except ElementClickInterceptedException as e:
            return
        except TimeoutException as e:
            return


    def __parse_diseases_result(self, eleList):
        result = []
        for ele in eleList[:self.LIMIT]:
            data = ele.text.split('\n')
            data = map(lambda s: s.strip(), data)
            result.append(data)
        return result