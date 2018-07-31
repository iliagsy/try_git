from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC


class BasePageElement(object):
    locator = ('', '')
    seconds = 30

    def __get__(self, obj, owner):
        '''return element'''
        client = obj.client
        seconds = self.seconds
        return WebDriverWait(client, seconds).until(
                lambda c: c.find_element(*self.locator))  


class BaseButtonElement(BasePageElement):
    def __get__(self, obj, owner):
        client = obj.client
        seconds = self.seconds
        WebDriverWait(client, seconds).until(
                EC.visibility_of_any_elements_located(self.locator))  
        return WebDriverWait(client, seconds).until(
                EC.element_to_be_clickable(self.locator))  


class BasePageElementListByIndex(object):
    locator = ('', '')
    seconds = 30
    index = None

    def __get__(self, obj, owner):
        client = obj.client
        seconds = self.seconds
        _l = WebDriverWait(client, seconds).until(
                lambda c: c.find_elements(*self.locator))
        if self.index is None:
            return _l
        return _l[self.index]


class OnLoadWindowClose(BasePageElement):
    locator = (By.CLASS_NAME, 'x-tool-close')
    seconds = 300


class FeatureTab(BasePageElement):
    locator = (By.PARTIAL_LINK_TEXT, "Patient's Features.")


class DiagTab(BasePageElementListByIndex):
    locator = (By.PARTIAL_LINK_TEXT, "Diagnosis.")
    index = -1


class SchInputBox(BasePageElement):
    locator = (By.TAG_NAME, 'input')


class SchInputButton(BaseButtonElement):
    locator = (By.XPATH, "//button[text()='search.']")


class SchResetButton(BaseButtonElement):
    locator = (By.XPATH, "//button[text()='reset.']")


class ClearHpoButton(BaseButtonElement):
    locator = (By.XPATH, "//button[text()='Clear.']")


class GetDiseasesButton(BaseButtonElement):
    locator = (By.XPATH, "//button[text()='Get diagnosis.']")


class DiseasesResult(BasePageElementListByIndex):
    locator = (By.CLASS_NAME, 'x-grid3-body')
    index = -1
    locator2 = (By.CLASS_NAME, 'x-grid3-row')
    index2 = None

    def __get__(self, obj, owner):
        client = obj.client
        seconds = self.seconds
        _l = WebDriverWait(client, seconds).until(
                lambda c: c.find_elements(*self.locator))
        body = _l[self.index]

        return WebDriverWait(body, seconds).until(
                lambda c: c.find_elements(*self.locator2))


class SchResult(BasePageElementListByIndex):
    locator = (By.CLASS_NAME, 'x-grid3-body')
    index = 0
    locator2 = (By.CLASS_NAME, 'x-grid3-row')

    def __get__(self, obj, owner):
        client = obj.client
        seconds = self.seconds
        _l = WebDriverWait(client, seconds).until(
                lambda c: c.find_elements(*self.locator))
        body = _l[self.index]
        WebDriverWait(body, seconds).until(
                lambda c: len(c.find_elements(*self.locator2)) < 10)
        return body.find_elements(*self.locator2)


class YesButton(BaseButtonElement):
    locator = (By.XPATH, "//button[text()='Yes']")
    seconds = 0.1


class DragHpoTarget(BasePageElementListByIndex):
    index = 1
    locator = (By.CLASS_NAME, 'x-grid3-scroller')


class FeaturesPrepared(BasePageElementListByIndex):
    locator = (By.CLASS_NAME, 'x-grid3-body')
    index = 1


class DiagTabClose(BasePageElementListByIndex):
    locator = (By.CLASS_NAME, 'x-tab-strip-close')
